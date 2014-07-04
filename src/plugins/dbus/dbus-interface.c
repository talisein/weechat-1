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
#include "dbus-interface.h"

struct t_dbus_interface
{
    struct t_hashtable *method_ht;
    struct t_hashtable *signal_ht;
    struct t_hashtable *property_ht;
    char *name;
    size_t ref_cnt;
    bool is_deprecated;
};

static void
weechat_dbus_interface_methods_free (struct t_hashtable *hashtable,
                                     const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_method_free ( (struct t_dbus_method*)value);
}

static void
weechat_dbus_interface_signals_free (struct t_hashtable *hashtable,
                                     const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_signal_free ( (struct t_dbus_signal*)value);
}

static void
weechat_dbus_interface_properties_free (struct t_hashtable *hashtable,
                                        const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_property_free ( (struct t_dbus_property*)value);
}


struct t_dbus_interface *
weechat_dbus_interface_new (const char *name,
                            bool is_deprecated)
{
    if (!name)
    {
        return NULL;
    }

    DBusError err;
    dbus_error_init (&err);
    if (!dbus_validate_interface(name, &err))
    {
        if (dbus_error_is_set (&err))
        {
            weechat_printf (NULL,
                            _("%s%s: Can't create interface %s: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            name, err.message);
            dbus_error_free (&err);  
        }
        return NULL;
    }

    struct t_dbus_interface *i = malloc (sizeof (struct t_dbus_interface));
    if (!i)
    {
        return NULL;
    }

    i->method_ht = weechat_hashtable_new (8, WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_POINTER,
                                          NULL, NULL);
    if (!i->method_ht)
    {
        free (i);
        return NULL;
    }

    i->signal_ht = weechat_hashtable_new (8, WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_POINTER,
                                          NULL, NULL);
    if (!i->signal_ht)
    {
        weechat_hashtable_free (i->method_ht);
        free (i);
        return NULL;
    }

    i->property_ht = weechat_hashtable_new (8, WEECHAT_HASHTABLE_STRING,
                                            WEECHAT_HASHTABLE_POINTER,
                                            NULL, NULL);
    if (!i->property_ht)
    {
        weechat_hashtable_free (i->method_ht);
        weechat_hashtable_free (i->signal_ht);
        free (i);
        return NULL;
    }

    i->name = strdup (name);
    if (!i->name)
    {
        weechat_hashtable_free (i->method_ht);
        weechat_hashtable_free (i->signal_ht);
        weechat_hashtable_free (i->property_ht);
        free (i);
        return NULL;
    }

    weechat_hashtable_set_pointer (i->method_ht, "callback_free_value",
                                  &weechat_dbus_interface_methods_free);
    weechat_hashtable_set_pointer (i->signal_ht, "callback_free_value",
                                  &weechat_dbus_interface_signals_free);
    weechat_hashtable_set_pointer (i->property_ht, "callback_free_value",
                                  &weechat_dbus_interface_properties_free);

    i->is_deprecated = is_deprecated;
    i->ref_cnt = 1;

    return i;
}

int
weechat_dbus_interface_add_method (struct t_dbus_interface *i,
                                  struct t_dbus_method *m)
{
    if (!i || !m)
    {
        return WEECHAT_RC_ERROR;
    }

    const char *name = weechat_dbus_method_get_name (m);

    if (weechat_hashtable_has_key (i->method_ht, name))
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_hashtable_item *item = weechat_hashtable_set (i->method_ht,
                                                           name, m);
    if (!item)
    {
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}

int
weechat_dbus_interface_add_signal (struct t_dbus_interface *i,
                                   struct t_dbus_signal *s)
{
    if (!i || !s)
    {
        return WEECHAT_RC_ERROR;
    }

    const char *name = weechat_dbus_signal_get_name (s);

    if (weechat_hashtable_has_key (i->signal_ht, name))
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_hashtable_item *item = weechat_hashtable_set (i->signal_ht,
                                                           name, s);
    if (!item)
    {
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}

int
weechat_dbus_interface_add_property(struct t_dbus_interface *i,
                                    struct t_dbus_property *p)
{
    if (!i || !p)
    {
        return WEECHAT_RC_ERROR;
    }

    const char *name = weechat_dbus_property_get_name (p);

    if (weechat_hashtable_has_key (i->property_ht, name))
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_hashtable_item *item = weechat_hashtable_set (i->property_ht,
                                                           name, p);
    if (!item)
    {
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}

const char *
weechat_dbus_interface_get_name (const struct t_dbus_interface *i)
{
    if (!i)
    {
        return NULL;
    }

    return i->name;
}

void
weechat_dbus_interface_unref (struct t_dbus_interface *i)
{
    if (!i)
    {
        return;
    }

    --i->ref_cnt;

    if (0 == i)
    {
        free (i->name);
        weechat_hashtable_free (i->method_ht);
        weechat_hashtable_free (i->signal_ht);
        weechat_hashtable_free (i->property_ht);
        free (i);
    }
}

void
weechat_dbus_interface_ref (const struct t_dbus_interface *i)
{
    if (!i)
    {
        return;
    }

    /* ref_cnt is mutable */
    size_t *cnt = (size_t*)&i->ref_cnt;

    ++(*cnt);
}

DBusHandlerResult
weechat_dbus_interface_handle_msg (const struct t_dbus_interface *i,
                                   struct t_dbus_object *o,
                                   DBusConnection *conn,
                                   DBusMessage *msg)
{
    const char *member = dbus_message_get_member (msg);
    if (!member)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    const struct t_dbus_method *m;
    m = (const struct t_dbus_method *)weechat_hashtable_get (i->method_ht,
                                                             member);
    if (!m)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    return weechat_dbus_method_handle_msg (m, o, conn, msg);
}

struct _callback_data
{
    xmlTextWriterPtr writer;
    bool had_error;
};

static void
_introspect_method_callback(void *data,
                            struct t_hashtable *hashtable,
                            const void *key,
                            const void *value)
{
    (void) hashtable;
    (void) key;
    struct _callback_data *callback_data = (struct _callback_data*)data;
    struct t_dbus_method *m = (struct t_dbus_method *)value;

    int res = weechat_dbus_method_introspect (m, callback_data->writer);
    if (WEECHAT_RC_ERROR == res)
    {
        callback_data->had_error = true;
    }
}

static void
_introspect_signal_callback(void *data,
                            struct t_hashtable *hashtable,
                            const void *key,
                            const void *value)
{
    (void) hashtable;
    (void) key;
    struct _callback_data *callback_data = (struct _callback_data*)data;
    struct t_dbus_signal *s = (struct t_dbus_signal *)value;

    int res = weechat_dbus_signal_introspect (s, callback_data->writer);
    if (WEECHAT_RC_ERROR == res)
    {
        callback_data->had_error = true;
    }
}

static void
_introspect_property_callback(void *data,
                              struct t_hashtable *hashtable,
                              const void *key,
                              const void *value)
{
    (void) hashtable;
    (void) key;
    struct _callback_data *callback_data = (struct _callback_data*)data;
    struct t_dbus_property *p = (struct t_dbus_property *)value;

    int res = weechat_dbus_property_introspect (p, callback_data->writer);
    if (WEECHAT_RC_ERROR == res)
    {
        callback_data->had_error = true;
    }
}

int
weechat_dbus_interface_introspect (struct t_dbus_interface *i,
                                   xmlTextWriterPtr writer)
{
    int rc;
    rc = xmlTextWriterStartElement (writer, BAD_CAST "interface");
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterWriteAttribute (writer,
                                       BAD_CAST "name",
                                       BAD_CAST i->name);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    struct _callback_data data = {writer, false};
    weechat_hashtable_map (i->method_ht,
                           &_introspect_method_callback,
                           &data);
    if (data.had_error)
    {
        return WEECHAT_RC_ERROR;
    }

    weechat_hashtable_map (i->signal_ht,
                           &_introspect_signal_callback,
                           &data);
    if (data.had_error)
    {
        return WEECHAT_RC_ERROR;
    }

    weechat_hashtable_map (i->property_ht,
                           &_introspect_property_callback,
                           &data);
    if (data.had_error)
    {
        return WEECHAT_RC_ERROR;
    }

    if (i->is_deprecated)
    {
        rc = xmlTextWriterStartElement (writer, BAD_CAST "annotation");
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }

        rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "name",
                                          BAD_CAST "org.freedesktop.DBus.Deprecated");
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }

        rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "value",
                                          BAD_CAST "true");
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }

        rc = xmlTextWriterEndElement (writer);
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }
    }

    rc = xmlTextWriterEndElement (writer);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}
