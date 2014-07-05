/*
 * Copyright (C) 2014 Andrew Potter <agpotter@gmail.com>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dbus/dbus.h>
#include <stdlib.h>
#include <string.h>
#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-object.h"

struct t_dbus_object
{
    const struct t_dbus_object *parent;
    struct t_hashtable         *interface_ht;
    struct t_hashtable         *children_ht;
    char                       *path;
    const void                 *obj;
    size_t                      ref_cnt;
};

static void
weechat_dbus_object_interfaces_free (struct t_hashtable *hashtable,
                                     const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_interface_unref ((struct t_dbus_interface*)value);
}

static void
weechat_dbus_object_objects_free (struct t_hashtable *hashtable,
                                  const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_object_unref ((struct t_dbus_object*)value);
}

struct t_dbus_object *
weechat_dbus_object_new (struct t_dbus_object *parent,
                         const char *path,
                         const void *obj)
{
    if (!path) return NULL;

    DBusError err;
    dbus_error_init (&err);
    if (!dbus_validate_path(path, &err))
    {
        if (dbus_error_is_set (&err))
        {
            weechat_printf (NULL,
                            _("%s%s: Can't create object %s: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            path, err.message);
            dbus_error_free (&err);  
        }
        return NULL;
    }

    struct t_dbus_object *o = malloc (sizeof (struct t_dbus_object));
    if (!o) return NULL;

    o->interface_ht = weechat_hashtable_new (8, WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_POINTER,
                                             NULL, NULL);
    if (!o->interface_ht) {
        free (o);
        return NULL;
    }
    o->children_ht = weechat_hashtable_new (8, WEECHAT_HASHTABLE_POINTER,
                                            WEECHAT_HASHTABLE_POINTER,
                                            NULL, NULL);
    if (!o->children_ht) {
        weechat_hashtable_free (o->interface_ht);
        free (o);
        return NULL;
    }

    o->path = strdup (path);
    if (!o->path)
    {
        weechat_hashtable_free (o->children_ht);
        weechat_hashtable_free (o->interface_ht);
        free (o);
        return NULL;
    }

    weechat_hashtable_set_pointer (o->interface_ht, "callback_free_value",
                                   &weechat_dbus_object_interfaces_free);

    weechat_hashtable_set_pointer (o->children_ht, "callback_free_value",
                                   &weechat_dbus_object_objects_free);

    o->obj = obj;
    o->parent = parent;
    o->ref_cnt = 1;

    return o;
}

static const char *
weechat_dbus_object_get_relative_path (const struct t_dbus_object *o)
{
    const char *out = strrchr (o->path, '/');
    return ++out;
}

int
weechat_dbus_object_add_interface (struct t_dbus_object *o,
                                   const struct t_dbus_interface *i)
{
    const char *iname = weechat_dbus_interface_get_name (i);

    if (weechat_hashtable_has_key (o->interface_ht, iname))
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_hashtable_item *item = weechat_hashtable_set (o->interface_ht, iname, i);
    if (!item)
    {
        return WEECHAT_RC_ERROR;
    }

    weechat_dbus_interface_ref (i);

    return WEECHAT_RC_OK;
}

int
weechat_dbus_object_add_child (struct t_dbus_object *parent,
                               const struct t_dbus_object *child)
{
    if (!parent || !child)
    {
        return WEECHAT_RC_ERROR;
    }

    if (weechat_hashtable_has_key (parent->children_ht, child))
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_hashtable_item *item = weechat_hashtable_set (parent->children_ht, child, child);
    if (!item)
    {
        return WEECHAT_RC_ERROR;
    }

    weechat_dbus_object_ref (child);

    return WEECHAT_RC_OK;
}

void *
weechat_dbus_object_get_object (const struct t_dbus_object *obj)
{
    if (!obj)
    {
        return NULL;
    }

    return (void *)obj->obj;
}

void
weechat_dbus_object_unref (struct t_dbus_object *o)
{
    if (!o)
    {
        return;
    }

    --o->ref_cnt;

    if (0 == o->ref_cnt)
    {
        weechat_hashtable_free (o->interface_ht);
        weechat_hashtable_free (o->children_ht);
        free (o->path);
    }
}

void
weechat_dbus_object_ref (const struct t_dbus_object *o)
{
    if (!o)
    {
        return;
    }

    /* ref_cnt is mutable */
    size_t *cnt = (size_t*)&o->ref_cnt;

    ++(*cnt);
}

static void
weechat_dbus_object_path_unregister (DBusConnection *conn,
                                     void *user_data)
{
    (void) conn;
    (void) user_data;
}

static DBusHandlerResult
weechat_dbus_object_path_message (DBusConnection *conn,
                                  DBusMessage *msg,
                                  void *user_data)
{
    if (DBUS_MESSAGE_TYPE_METHOD_CALL != dbus_message_get_type (msg))
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    struct t_dbus_object *o = (struct t_dbus_object *)user_data;
    const char *iface_name = dbus_message_get_interface (msg);
    if (!iface_name)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    struct t_dbus_interface *iface;
    iface = (struct t_dbus_interface *)weechat_hashtable_get (o->interface_ht,
                                                              iface_name);
    if (!iface)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    return weechat_dbus_interface_handle_msg (iface, o, conn, msg);
}

int
weechat_dbus_object_register (struct t_dbus_object *obj, DBusConnection *conn)
{
    if (!obj || !conn)
    {
        return WEECHAT_RC_ERROR;
    }

    DBusObjectPathVTable vtable;
    memset (&vtable, 0, sizeof (vtable));
    vtable.unregister_function = &weechat_dbus_object_path_unregister;
    vtable.message_function = &weechat_dbus_object_path_message;
    
    DBusError err;
    dbus_error_init (&err);

    if (!dbus_connection_try_register_object_path (conn,
                                                   obj->path,
                                                   &vtable,
                                                   obj,
                                                   &err))
    {
        if (dbus_error_is_set (&err)) {
            weechat_printf (NULL,
                            _("%s%s: Error registering %s to session DBus: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            obj->path, err.message);
            dbus_error_free (&err);
        }
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}


struct _callback_data
{
    xmlTextWriterPtr writer;
    bool had_error;
};

static void
_introspect_interface_callback (void *data,
                                struct t_hashtable *hashtable,
                                const void *key,
                                const void *value)
{
    (void) hashtable;
    (void) key;
    struct _callback_data *callback_data = (struct _callback_data*)data;
    struct t_dbus_interface *iface = (struct t_dbus_interface *)value;
    int res = weechat_dbus_interface_introspect (iface, callback_data->writer);
    if (WEECHAT_RC_ERROR == res)
    {
        callback_data->had_error = true;
    }
}

static void
_introspect_object_callback (void *data,
                             struct t_hashtable *hashtable,
                             const void *key,
                             const void *value)
{
    (void) hashtable;
    (void) key;
    struct _callback_data *callback_data = (struct _callback_data*)data;
    struct t_dbus_object *obj = (struct t_dbus_object *)value;
    int res = weechat_dbus_object_introspect (obj, callback_data->writer, false);
    if (WEECHAT_RC_ERROR == res)
    {
        callback_data->had_error = true;
    }
}

int
weechat_dbus_object_introspect(struct t_dbus_object *obj,
                               xmlTextWriterPtr writer,
                               bool is_root)
{
    int rc;
    rc = xmlTextWriterStartElement (writer, BAD_CAST "node");
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    if (!is_root)
    {
        const char *rel_path = weechat_dbus_object_get_relative_path (obj);
        rc = xmlTextWriterWriteAttribute (writer,
                                          BAD_CAST "name",
                                          BAD_CAST rel_path);
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }
    }

    struct _callback_data data = {writer, false};
    weechat_hashtable_map (obj->interface_ht,
                           &_introspect_interface_callback,
                           &data);
    if (data.had_error)
    {
        return WEECHAT_RC_ERROR;
    }

    weechat_hashtable_map (obj->children_ht,
                           &_introspect_object_callback,
                           &data);
    if (data.had_error)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterEndElement (writer);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}

struct t_dbus_interface *
weechat_dbus_object_get_interface (struct t_dbus_object *obj,
                                   const char *interface_name)
{
    if (!obj || !interface_name)
    {
        return NULL;
    }

    struct t_dbus_interface *iface;
    iface = weechat_hashtable_get (obj->interface_ht, interface_name);
    
    return iface;
}

