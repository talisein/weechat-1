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

struct t_dbus_object_factory
{
    struct t_hashtable *interface_cache_ht;
    struct t_hashtable *buffers_ht;
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
weechat_dbus_object_factory_buffers_free (struct t_hashtable *hashtable,
                                          const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_object_unref ((struct t_dbus_object*)value);
}

struct t_dbus_object_factory *
weechat_dbus_object_factory_new (void)
{
    struct t_dbus_object_factory *factory;
    factory = malloc (sizeof (struct t_dbus_object_factory));
    if (!factory)
    {
        return NULL;
    }

    factory->interface_cache_ht = weechat_hashtable_new (32,
                                                         WEECHAT_HASHTABLE_STRING,
                                                         WEECHAT_HASHTABLE_POINTER,
                                                         NULL, NULL);
    if (!factory->interface_cache_ht)
    {
        free (factory);
        return NULL;
    }

    factory->buffers_ht = weechat_hashtable_new (64, WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_POINTER,
                                                 NULL, NULL);
    if (!factory->buffers_ht)
    {
        weechat_hashtable_free (factory->interface_cache_ht);
        free (factory);
        return NULL;
    }

    weechat_hashtable_set_pointer (factory->interface_cache_ht,
                                   "callback_free_value",
                                   &weechat_dbus_object_factory_interfaces_free);
    weechat_hashtable_set_pointer (factory->buffers_ht,
                                   "callback_free_value",
                                   &weechat_dbus_object_factory_buffers_free);

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
                                         struct t_gui_buffer *buffer,
                                         DBusConnection* conn)
{
    if (!factory || !buffer)
    {
        return WEECHAT_RC_ERROR;
    }

    int res;
    struct t_dbus_object *o;
    struct t_dbus_interface *iface;
    struct t_hashtable_item *item;
    const char path_prefix[] = "/org/weechat/Buffer/";
    const char *full_name;
    char *path;
    char *buf;
    size_t buf_size;
    size_t path_prefix_strlen = sizeof(path_prefix) - 1;

    /* create sanitized path name */
    full_name = weechat_buffer_get_string (buffer, "full_name");
    buf_size = path_prefix_strlen + strlen (full_name) + 1;
    buf = malloc (buf_size);
    if (!buf)
    {
        return WEECHAT_RC_ERROR;
    }
    snprintf (buf, buf_size, "%s%s", path_prefix, full_name);
    path = _sanitize_dbus_path (buf);
    free (buf);
    if (!path)
    {
        return WEECHAT_RC_ERROR;
    }

    /* Make the bare object */
    o = weechat_dbus_object_new (NULL, path, full_name);
    free (path);
    if (!o)
    {
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

    res = weechat_dbus_object_register (o, conn);
    if (WEECHAT_RC_ERROR == res)
    {
        goto error;
    }

    return WEECHAT_RC_OK;

error:
        weechat_dbus_object_unref (o);
        weechat_buffer_set (buffer, "localvar_del_"
                            DBUS_BUFFER_ORIG_FULL_NAME_LOCALVAR,
                            NULL);
        return WEECHAT_RC_ERROR;
}

static int
_hook_buffer_updates (struct t_dbus_object_factory *factory)
{
    (void) factory;
    return 0;
}

