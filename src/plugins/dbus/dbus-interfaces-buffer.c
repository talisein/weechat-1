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
#include "dbus.h"
#include "dbus-interfaces-buffer.h"
#include "dbus-object.h"
#include "dbus-interface.h"

static DBusHandlerResult
weechat_dbus_interfaces_buffer_command (struct t_dbus_object *o,
                                        DBusConnection *conn,
                                        DBusMessage *msg)
{
    DBusError err;
    dbus_error_init (&err);
    DBusMessage *reply;
    const char *cmd;
    dbus_bool_t res;

    if (!dbus_message_get_args (msg, &err,
                                DBUS_TYPE_STRING, &cmd,
                                DBUS_TYPE_INVALID))
    {
        if (dbus_error_is_set (&err))
        {
            reply = dbus_message_new_error_printf (msg, DBUS_ERROR_INVALID_ARGS,
                                                   WEECHAT_DBUS_INTERFACES_BUFFER
                                                   ".Command requires signature "
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

    const char *full_name = weechat_dbus_object_get_object (o);
    struct t_gui_buffer *buf = weechat_buffer_search ("==", full_name);
    if (!buf)
    {
        reply = dbus_message_new_error_printf (msg, DBUS_ERROR_FAILED,
                                               "Unknown buffer name %s",
                                               full_name);
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

    weechat_command (buf, cmd);

    return DBUS_HANDLER_RESULT_HANDLED;
}


struct t_dbus_interface*
weechat_dbus_interfaces_buffer_new (void)
{
    struct t_dbus_interface *iface;
    struct t_dbus_method *m;
    int res;

    iface = weechat_dbus_interface_new (WEECHAT_DBUS_INTERFACES_BUFFER,
                                        NULL,
                                        false);
    if (NULL == iface)
    {
        return NULL;
    }

    m = weechat_dbus_method_new ("Command",
                                 &weechat_dbus_interfaces_buffer_command,
                                 false, true);
    if (!m)
    {
        goto error;
    }

    res = weechat_dbus_method_add_arg (m, "command",
                                       DBUS_TYPE_STRING_AS_STRING,
                                       WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    
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

    return iface;

error:
    weechat_dbus_interface_unref (iface);
    return NULL;
}

