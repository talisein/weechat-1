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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dbus/dbus.h>
#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-object-factory.h"
#include "dbus-object.h"
#include "dbus-interfaces-peer.h"
#include "dbus-interfaces-introspectable.h"
#include "dbus-interfaces-properties.h"
#include "dbus-interfaces-object-manager.h"
#include "dbus-interfaces-buffer.h"

#define DBUS_BUFFER_ORIG_FULL_NAME_LOCALVAR "dbus_orig_full_name"

/* 18446744073709551615 is 20 characters. */
#define SIZE_MAX_STRLEN 21

struct t_dbus_object_factory
{
    DBusConnection       *conn;
    struct t_hashtable   *interface_cache_ht;
    struct t_hashtable   *buffers_ht;
    struct t_dbus_object *root;
    struct t_dbus_object *buffers;
    size_t                buf_cnt;
};

static char *
_sanitize_dbus_path(const char *);

static int
_add_standard_interfaces (struct t_dbus_object_factory *, struct t_dbus_object *);

static int
_hook_buffer_updates (struct t_dbus_object_factory *);

static void
weechat_dbus_object_factory_interfaces_free (struct t_hashtable *hashtable,
                                             const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_interface_unref ((struct t_dbus_interface*)value);
}

static void
weechat_dbus_object_factory_buffers_free_value (struct t_hashtable *hashtable,
                                                const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_object_unref ((struct t_dbus_object*)value);
}

static void
weechat_dbus_object_factory_buffers_free_key (struct t_hashtable *hashtable,
                                              void *key, const void *value)
{
    (void) hashtable;
    (void) value;

    free (key);
}

struct t_dbus_object_factory *
weechat_dbus_object_factory_new (DBusConnection *conn)
{
    int res;

    if (!conn)
    {
        return NULL;
    }

    struct t_dbus_object_factory *factory;
    factory = malloc (sizeof (struct t_dbus_object_factory));
    if (!factory)
    {
        return NULL;
    }

    factory->root = weechat_dbus_object_new (NULL, "/org/weechat", NULL);
    if (!factory->root)
    {
        free (factory);
        return NULL;
    }

    factory->buffers = weechat_dbus_object_new (factory->root, "/org/weechat/buffer", NULL);
    if (!factory->buffers)
    {
        weechat_dbus_object_unref (factory->root);
        free (factory);
        return NULL;
    }
    factory->interface_cache_ht = weechat_hashtable_new (32,
                                                         WEECHAT_HASHTABLE_STRING,
                                                         WEECHAT_HASHTABLE_POINTER,
                                                         NULL, NULL);
    if (!factory->interface_cache_ht)
    {
        weechat_dbus_object_unref (factory->buffers);
        weechat_dbus_object_unref (factory->root);
        free (factory);
        return NULL;
    }

    factory->buffers_ht = weechat_hashtable_new (64, WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_POINTER,
                                                 NULL, NULL);
    if (!factory->buffers_ht)
    {
        weechat_dbus_object_unref (factory->buffers);
        weechat_dbus_object_unref (factory->root);
        weechat_hashtable_free (factory->interface_cache_ht);
        free (factory);
        return NULL;
    }

    weechat_hashtable_set_pointer (factory->interface_cache_ht,
                                   "callback_free_value",
                                   &weechat_dbus_object_factory_interfaces_free);
    weechat_hashtable_set_pointer (factory->buffers_ht,
                                   "callback_free_value",
                                   &weechat_dbus_object_factory_buffers_free_value);
    weechat_hashtable_set_pointer (factory->buffers_ht,
                                   "callback_free_key",
                                   &weechat_dbus_object_factory_buffers_free_key);

    factory->buf_cnt = 0;
    factory->conn = conn;

    res = _add_standard_interfaces (factory, factory->root);
    if (WEECHAT_RC_OK != res)
    {
        weechat_dbus_object_factory_free (factory);
        return NULL;
    }

    res = _add_standard_interfaces (factory, factory->buffers);
    if (WEECHAT_RC_OK != res)
    {
        weechat_dbus_object_factory_free (factory);
        return NULL;
    }

    res = weechat_dbus_object_register (factory->root, conn);
    if (WEECHAT_RC_OK != res)
    {
        weechat_dbus_object_factory_free (factory);
        return NULL;
    }

    res = weechat_dbus_object_register (factory->buffers, conn);
    if (WEECHAT_RC_OK != res)
    {
        weechat_dbus_object_factory_free (factory);
        return NULL;
    }

    return factory;
}

void
weechat_dbus_object_factory_free (struct t_dbus_object_factory *factory)
{
    if (!factory)
    {
        return;
    }

    weechat_hashtable_free (factory->interface_cache_ht);
    weechat_hashtable_free (factory->buffers_ht);
    weechat_dbus_object_unref (factory->buffers);
    weechat_dbus_object_unref (factory->root);
    free (factory);
}

static char *
_sanitize_dbus_path(const char *in)
{
    size_t out_size = strlen(in)+1;
    char *out = malloc(out_size);
    if (!out)
        return NULL;
    const char *out_end = out + out_size;
    const char *ip;
    char *op;

    for (ip = in, op = out; op < out_end && *ip; ++ip)
    {
        if (isascii(*ip) && (isalnum(*ip) || '/' == *ip || '_' == *ip))
        {
            *op = *ip;
        }
        else
        {
            *op = '_';
        }
        ++op;
    }

    /* ensure termination */
    if (op == out_end)
        --op;
    *op = '\0';

    /* Can't end in trailing slash */
    if (op != out)
    {
        --op;
        if ('/' == *op)
            *op = '\0';
    }

    return out;
}

static int
_add_standard_interfaces (struct t_dbus_object_factory *factory,
                          struct t_dbus_object *object)
{
    struct t_dbus_interface *iface;
    struct t_hashtable_item *item;
    int res;

    /* org.freedesktop.DBus.Peer */
    iface = weechat_hashtable_get (factory->interface_cache_ht,
                                   DBUS_INTERFACE_PEER);
    if (NULL == iface)
    {
        iface = weechat_dbus_interfaces_peer_new ();
        if (NULL == iface)
        {
            return WEECHAT_RC_ERROR;
        }

        item = weechat_hashtable_set (factory->interface_cache_ht,
                                      DBUS_INTERFACE_PEER, iface);
        if (!item)
        {
            weechat_dbus_interface_unref (iface);
            return WEECHAT_RC_ERROR;
        }
    }

    res = weechat_dbus_object_add_interface (object, iface);
    if (WEECHAT_RC_ERROR == res)
    {
        return WEECHAT_RC_ERROR;
    }

    /* org.freedesktop.DBus.Introspectable */
    iface = weechat_hashtable_get (factory->interface_cache_ht,
                                   DBUS_INTERFACE_INTROSPECTABLE);

    if (NULL == iface)
    {
        iface = weechat_dbus_interfaces_introspectable_new ();
        if (NULL == iface)
        {
            return WEECHAT_RC_ERROR;
        }

        item = weechat_hashtable_set (factory->interface_cache_ht,
                                      DBUS_INTERFACE_INTROSPECTABLE,
                                      iface);
        if (!item)
        {
            weechat_dbus_interface_unref (iface);
            return WEECHAT_RC_ERROR;
        }
    }

    res = weechat_dbus_object_add_interface (object, iface);
    if (WEECHAT_RC_ERROR == res)
    {
        return WEECHAT_RC_ERROR;
    }

    /* org.freedesktop.DBus.Properties */
    iface = weechat_hashtable_get (factory->interface_cache_ht,
                                   DBUS_INTERFACE_PROPERTIES);

    if (NULL == iface)
    {
        iface = weechat_dbus_interfaces_properties_new ();
        if (NULL == iface)
        {
            return WEECHAT_RC_ERROR;
        }

        item = weechat_hashtable_set (factory->interface_cache_ht,
                                      DBUS_INTERFACE_PROPERTIES,
                                      iface);
        if (!item)
        {
            weechat_dbus_interface_unref (iface);
            return WEECHAT_RC_ERROR;
        }
    }

    res = weechat_dbus_object_add_interface (object, iface);
    if (WEECHAT_RC_ERROR == res)
    {
        return WEECHAT_RC_ERROR;
    }

    /* org.freedesktop.DBus.ObjectManager */
    /*
    iface = weechat_hashtable_get (factory->interface_cache_ht,
                                   WEECHAT_DBUS_INTERFACES_OBJECT_MANAGER);
    if (NULL == iface)
    {
        iface = weechat_dbus_interfaces_object_manager_new ();
        if (NULL == iface)
        {
            return WEECHAT_RC_ERROR;
        }

        item = weechat_hashtable_set (factory->interface_cache_ht,
                                      WEECHAT_DBUS_INTERFACES_OBJECT_MANAGER,
                                      iface);
        if (!item)
        {
            weechat_dbus_interface_unref (iface);
            return WEECHAT_RC_ERROR;
        }
    }

    res = weechat_dbus_object_add_interface (object, iface);
    if (WEECHAT_RC_ERROR == res)
    {
        return WEECHAT_RC_ERROR;
    }
    */

    return WEECHAT_RC_OK;
}

int
weechat_dbus_object_factory_make_buffer (struct t_dbus_object_factory *factory,
                                         struct t_gui_buffer *buffer)
{
    if (!factory || !buffer)
    {
        return WEECHAT_RC_ERROR;
    }

    int res;
    struct t_dbus_object *o;
    struct t_dbus_interface *iface;
    struct t_hashtable_item *item;
    static const char   path_prefix[] = "/org/weechat/buffer/";
    static const size_t path_size     = (SIZE_MAX_STRLEN + sizeof(path_prefix));
    char path[path_size];
    char *full_name = strdup (weechat_buffer_get_string (buffer, "full_name"));
    if (!full_name)
    {
        weechat_printf (NULL,
                        _("%s%s: Couldn't get buffer name for %p"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME, buffer);
        return WEECHAT_RC_ERROR;
    }

    /* Make the bare object */
    res = snprintf (path, path_size, "%s%zu", path_prefix, factory->buf_cnt++);
    if (res < 0 || (size_t)res >= path_size)
    {
        weechat_printf (NULL,
                        _("%s%s: Not enough space to create path for buffer %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME, full_name);
        return WEECHAT_RC_ERROR;
    }

    o = weechat_dbus_object_new (factory->buffers, path, full_name);
    if (!o)
    {
        weechat_printf (NULL,
                        _("%s%s: Unable to create dbus object for %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME, full_name);
        return WEECHAT_RC_ERROR;
    }

    /* Store the current full_name so we can find it on rename */
    weechat_buffer_set (buffer, "localvar_set_"
                        DBUS_BUFFER_ORIG_FULL_NAME_LOCALVAR,
                        full_name);

    res = _add_standard_interfaces (factory, o);
    if (WEECHAT_RC_ERROR == res)
    {
        goto error;
    }
    
    /* org.weechat.Buffer */
    iface = weechat_hashtable_get (factory->interface_cache_ht,
                                   WEECHAT_DBUS_INTERFACES_BUFFER);
    if (NULL == iface)
    {
        iface = weechat_dbus_interfaces_buffer_new ();
        if (NULL == iface)
        {
            goto error;
        }

        item = weechat_hashtable_set (factory->interface_cache_ht,
                                      WEECHAT_DBUS_INTERFACES_BUFFER, iface);
        if (!item)
        {
            weechat_dbus_interface_unref (iface);
            goto error;
        }
    }

    res = weechat_dbus_object_add_interface (o, iface);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_interface_unref (iface);
        goto error;
    }

    res = weechat_dbus_object_register (o, factory->conn);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_printf (NULL,
                        _("%s%s: Unable to register dbus object for %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME, full_name);
        goto error;
    }

    item = weechat_hashtable_set (factory->buffers_ht, full_name, o);
    if (!item)
    {
        weechat_printf (NULL,
                        _("%s%s: Unable to map dbus object for %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME, full_name);
        goto error;
    }

    return WEECHAT_RC_OK;

error:
        weechat_dbus_object_unref (o);
        weechat_buffer_set (buffer, "localvar_del_"
                            DBUS_BUFFER_ORIG_FULL_NAME_LOCALVAR,
                            NULL);
        free (full_name);
        return WEECHAT_RC_ERROR;
}

int
weechat_dbus_object_factory_make_all_buffers (struct t_dbus_object_factory *factory)
{
    struct t_hdata *buffer_hdata = weechat_hdata_get ("buffer");
    struct t_gui_buffer *buffers = weechat_hdata_get_list (buffer_hdata,
                                                           "gui_buffers");
    int rc;

    while (buffers)
    {
        rc = weechat_dbus_object_factory_make_buffer (factory, buffers);
        if (WEECHAT_RC_OK != rc)
        {
            const char *full_name = weechat_buffer_get_string (buffers, "full_name");
            weechat_printf (NULL,
                            _("%s%s: Error adding buffer %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME, full_name);

            return WEECHAT_RC_ERROR;
        }

        buffers = weechat_hdata_pointer (buffer_hdata, buffers, "next_buffer");
    }

    return _hook_buffer_updates(factory);
}

static int
_hook_buffer_opened_cb (void *data, const char *signal, const char *type_data, void *signal_data)
{
    (void) type_data;
    (void) signal;

    struct t_dbus_object_factory *factory = (struct t_dbus_object_factory*)data;
    struct t_gui_buffer *buffer = (struct t_gui_buffer*)signal_data;

    const char *full_name = weechat_buffer_get_string (buffer, "full_name");
    weechat_printf (NULL, "Buffer %s opening...", full_name);

    int rc = weechat_dbus_object_factory_make_buffer (factory, buffer);
    if (WEECHAT_RC_OK != rc)
    {
        weechat_printf (NULL,
                        _("%s%s: Error creating dbus object for buffer %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME, full_name);
    }

    return WEECHAT_RC_OK;
}

static int
_hook_buffer_closing_cb (void *data, const char *signal, const char *type_data, void *signal_data)
{
    (void) type_data;
    (void) signal;

    struct t_dbus_object_factory *factory = (struct t_dbus_object_factory*)data;
    struct t_gui_buffer *buffer = (struct t_gui_buffer*)signal_data;
    const char *full_name = weechat_buffer_get_string (buffer, "full_name");

    weechat_hashtable_remove (factory->buffers_ht, full_name);

    return WEECHAT_RC_OK;
}

static int
_hook_buffer_renamed_cb (void *data, const char *signal, const char *type_data, void *signal_data)
{
    (void) type_data;
    (void) signal;

    struct t_dbus_object_factory *factory = (struct t_dbus_object_factory*)data;
    struct t_gui_buffer *buffer = (struct t_gui_buffer*)signal_data;
    const char *new_name = weechat_buffer_get_string (buffer, "full_name");
    const char *old_name = weechat_buffer_get_string (buffer, "localvar_"
                                                      DBUS_BUFFER_ORIG_FULL_NAME_LOCALVAR);

    struct t_dbus_object *object = weechat_hashtable_get (factory->buffers_ht, old_name);
    if (!object)
    {
        weechat_printf (NULL,
                        _("%s%s: Error renaming dbus object %s to %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                        old_name, new_name);
        return WEECHAT_RC_OK;
    }

    /* ref so we don't unregister from bus when removed from ht */
    weechat_dbus_object_ref (object);

    /* Remove old name from ht, re-insert with new name */
    weechat_hashtable_remove (factory->buffers_ht, old_name);
    weechat_hashtable_set (factory->buffers_ht, new_name, object);
    weechat_dbus_object_unref (object);

    /* Set new name on buffer */
    weechat_buffer_set (buffer, "localvar_set_"
                        DBUS_BUFFER_ORIG_FULL_NAME_LOCALVAR,
                        new_name);

    return WEECHAT_RC_OK;
}

static int
_hook_buffer_updates (struct t_dbus_object_factory *factory)
{
    weechat_hook_signal ("buffer_opened", &_hook_buffer_opened_cb, factory);
    weechat_hook_signal ("buffer_closing", &_hook_buffer_closing_cb, factory);
    weechat_hook_signal ("buffer_renamed", &_hook_buffer_renamed_cb, factory);

    return WEECHAT_RC_OK;
}

