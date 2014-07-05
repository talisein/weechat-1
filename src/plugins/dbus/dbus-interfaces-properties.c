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

#include <stdlib.h>
#include <dbus/dbus.h>
#include "../weechat-plugin.h"
#include "dbus-interfaces-properties.h"
#include "dbus-object.h"
#include "dbus-interface.h"

static DBusHandlerResult
weechat_dbus_interfaces_properties_get (struct t_dbus_object *o,
                                        DBusConnection *conn,
                                        DBusMessage *msg)
{
    DBusError err;
    dbus_error_init (&err);
    const char *interface_name, *property_name;
    DBusMessage *reply;
    dbus_bool_t res;

    /* Get the arguments */
    if (!dbus_message_get_args (msg, &err,
                                DBUS_TYPE_STRING, &interface_name,
                                DBUS_TYPE_STRING, &property_name,
                                DBUS_TYPE_INVALID))
    {
        if (dbus_error_is_set (&err))
        {
            reply = dbus_message_new_error_printf (msg, DBUS_ERROR_INVALID_ARGS,
                                                   DBUS_INTERFACE_PROPERTIES
                                                   ".Get requires signature ss: %s",
                                                   err.message);
            dbus_error_free (&err);
        }
        else
        {
            reply = dbus_message_new_error_printf (msg, DBUS_ERROR_INVALID_ARGS,
                                                   DBUS_INTERFACE_PROPERTIES
                                                   ".Get requires signature ss");
        }
        
        if (!reply)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        res = dbus_connection_send (conn, reply, NULL);
        dbus_message_unref (reply);
        if (!res)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    /* Find the interface */
    struct t_dbus_interface *iface;
    iface = weechat_dbus_object_get_interface (o, interface_name);
    if (!iface)
    {
        reply = dbus_message_new_error_printf (msg, DBUS_ERROR_UNKNOWN_INTERFACE,
                                               "Unknown interface: %s",
                                               interface_name);
        if (!reply)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        res = dbus_connection_send (conn, reply, NULL);
        dbus_message_unref (reply);
        if (!res)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return weechat_dbus_interface_property_get (iface, o, property_name,
                                                conn, msg);
}

static DBusHandlerResult
weechat_dbus_interfaces_properties_set (struct t_dbus_object *o,
                                        DBusConnection *conn,
                                        DBusMessage *msg)
{
    const char *interface_name, *property_name;
    DBusMessageIter iter;
    dbus_message_iter_init (msg, &iter);
    DBusBasicValue val;
    DBusMessage *reply;
    dbus_bool_t res;

    /* Get the arguments */
    /* 1/3 STRING interface_name */
    if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type (&iter))
    {
        goto error_invalid_sig;
    }
    dbus_message_iter_get_basic (&iter, &val);
    interface_name = val.str;
    res = dbus_message_iter_next (&iter);
    if (!res)
    {
        goto error_invalid_sig;
    }
    /* 2/3 STRING property_name */
    if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type (&iter))
    {
        goto error_invalid_sig;
    }
    dbus_message_iter_get_basic (&iter, &val);
    property_name = val.str;
    res = dbus_message_iter_next (&iter);
    if (!res)
    {
        goto error_invalid_sig;
    }
    /* 3/3 VARIANT value */
    if (dbus_message_iter_has_next (&iter))
    {
        goto error_invalid_sig;
    }
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type (&iter))
    {
        goto error_invalid_sig;
    }
    DBusMessageIter iter_variant;
    dbus_message_iter_recurse (&iter, &iter_variant);
    
    /* Find the interface */
    struct t_dbus_interface *iface;
    iface = weechat_dbus_object_get_interface (o, interface_name);
    if (!iface)
    {
        reply = dbus_message_new_error_printf (msg, DBUS_ERROR_UNKNOWN_INTERFACE,
                                               "Unknown interface: %s",
                                               interface_name);
        if (!reply)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        res = dbus_connection_send (conn, reply, NULL);
        dbus_message_unref (reply);
        if (!res)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    /* Call the handler */
    return weechat_dbus_interface_property_set (iface, o, property_name,
                                                conn, msg, &iter_variant);

error_invalid_sig:
    reply = dbus_message_new_error_printf (msg, DBUS_ERROR_INVALID_ARGS,
                                           DBUS_INTERFACE_PROPERTIES
                                           ".Set requires signature ssv");
        
    if (!reply)
    {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    res = dbus_connection_send (conn, reply, NULL);
    dbus_message_unref (reply);
    if (!res)
    {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
weechat_dbus_interfaces_properties_get_all (struct t_dbus_object *o,
                                            DBusConnection *conn,
                                            DBusMessage *msg)
{
    DBusError err;
    dbus_error_init (&err);
    const char *interface_name;
    DBusMessage *reply;
    dbus_bool_t res;

    /* Get the arguments */
    if (!dbus_message_get_args (msg, &err,
                                DBUS_TYPE_STRING, &interface_name,
                                DBUS_TYPE_INVALID))
    {
        if (dbus_error_is_set (&err))
        {
            reply = dbus_message_new_error_printf (msg, DBUS_ERROR_INVALID_ARGS,
                                                   DBUS_INTERFACE_PROPERTIES
                                                   ".GetAll requires signature "
                                                   "s: %s",
                                                   err.message);
            dbus_error_free (&err);
        }
        else
        {
            reply = dbus_message_new_error_printf (msg, DBUS_ERROR_INVALID_ARGS,
                                                   DBUS_INTERFACE_PROPERTIES
                                                   ".GetAll requires signature s");
        }
        
        if (!reply)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        res = dbus_connection_send (conn, reply, NULL);
        dbus_message_unref (reply);
        if (!res)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    /* Find the interface */
    struct t_dbus_interface *iface;
    iface = weechat_dbus_object_get_interface (o, interface_name);
    if (!iface)
    {
        reply = dbus_message_new_error_printf (msg, DBUS_ERROR_UNKNOWN_INTERFACE,
                                               "Unknown interface: %s",
                                               interface_name);
        if (!reply)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        res = dbus_connection_send (conn, reply, NULL);
        dbus_message_unref (reply);
        if (!res)
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return weechat_dbus_interface_property_get_all (iface, o, conn, msg);
}

struct t_dbus_interface*
weechat_dbus_interfaces_properties_new (void)
{
    struct t_dbus_interface *iface;
    struct t_dbus_method *m;
    struct t_dbus_signal *s;
    int res;

    iface = weechat_dbus_interface_new (DBUS_INTERFACE_PROPERTIES,
                                        NULL,
                                        false);
    if (NULL == iface)
    {
        return NULL;
    }

    /* org.freedesktop.DBus.Properties.Get */
    m = weechat_dbus_method_new ("Get",
                                 &weechat_dbus_interfaces_properties_get,
                                 false, false);
    if (!m)
    {
        goto error;
    }
    res = weechat_dbus_method_add_arg (m, "interface_name",
                                       DBUS_TYPE_STRING_AS_STRING,
                                       WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_method_add_arg (m, "property_name",
                                       DBUS_TYPE_STRING_AS_STRING,
                                       WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_method_add_arg (m, "value",
                                       DBUS_TYPE_VARIANT_AS_STRING,
                                       WEECHAT_DBUS_ARGUMENT_DIRECTION_OUT);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_interface_add_method (iface, m);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }

    /* org.freedesktop.DBus.Properties.Set */
    m = weechat_dbus_method_new ("Set",
                                 &weechat_dbus_interfaces_properties_set,
                                 false, false);
    if (!m)
    {
        goto error;
    }
    res = weechat_dbus_method_add_arg (m, "interface_name",
                                       DBUS_TYPE_STRING_AS_STRING,
                                       WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_method_add_arg (m, "property_name",
                                       DBUS_TYPE_STRING_AS_STRING,
                                       WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_method_add_arg (m, "value",
                                       DBUS_TYPE_VARIANT_AS_STRING,
                                       WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_interface_add_method (iface, m);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }

    /* org.freedesktop.DBus.Properties.GetAll */
    m = weechat_dbus_method_new ("GetAll",
                                 &weechat_dbus_interfaces_properties_get_all,
                                 false, false);
    if (!m)
    {
        goto error;
    }
    res = weechat_dbus_method_add_arg(m, "interface_name",
                                          DBUS_TYPE_STRING_AS_STRING,
                                          WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_method_add_arg(m, "property_name",
                                      DBUS_TYPE_ARRAY_AS_STRING
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                      DBUS_TYPE_STRING_AS_STRING
                                      DBUS_TYPE_VARIANT_AS_STRING
                                      DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                      WEECHAT_DBUS_ARGUMENT_DIRECTION_OUT);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_interface_add_method (iface, m);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }

    /* org.freedesktop.DBus.Properties.PropertiesChanged */
    s = weechat_dbus_signal_new("PropertiesChanged", false);
    if (!s)
    {
        goto error;
    }
    res = weechat_dbus_signal_add_arg(s, "interface_name",
                                      DBUS_TYPE_STRING_AS_STRING);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_signal_free (s);
        goto error;
    }
    res = weechat_dbus_signal_add_arg(s, "changed_properties",
                                      DBUS_TYPE_ARRAY_AS_STRING
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                      DBUS_TYPE_STRING_AS_STRING
                                      DBUS_TYPE_VARIANT_AS_STRING
                                      DBUS_DICT_ENTRY_END_CHAR_AS_STRING);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_signal_free (s);
        goto error;
    }
    res = weechat_dbus_signal_add_arg(s, "invalidated_properties",
                                      DBUS_TYPE_ARRAY_AS_STRING
                                      DBUS_TYPE_STRING_AS_STRING);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_signal_free (s);
        goto error;
    }

    res = weechat_dbus_interface_add_signal (iface, s);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_signal_free (s);
        goto error;
    }

    return iface;

error:
    weechat_dbus_interface_unref (iface);
    return NULL;
}
