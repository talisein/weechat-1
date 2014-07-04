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
#include "dbus-interfaces-peer.h"
#include "dbus-interface.h"

static DBusHandlerResult
weechat_dbus_interfaces_peer_ping (struct t_dbus_object *o,
                                   DBusConnection *conn,
                                   DBusMessage *msg)
{
    (void) o;
    dbus_bool_t res;
    DBusMessage *reply = dbus_message_new_method_return (msg);
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
weechat_dbus_interfaces_peer_get_machine_id (struct t_dbus_object *o,
                                             DBusConnection *conn,
                                             DBusMessage *msg)
{
    (void) o;
    dbus_bool_t res;

    char *machine_id = dbus_get_local_machine_id ();
    if (!machine_id)
    {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    DBusMessage *reply = dbus_message_new_method_return (msg);
    if (!reply)
    {
        dbus_free (machine_id);
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    res = dbus_message_append_args (reply,
                                    DBUS_TYPE_STRING, &machine_id,
                                    DBUS_TYPE_INVALID);
    if (res)
    {
        res = dbus_connection_send (conn, reply, NULL);
    }

    dbus_free (machine_id);
    dbus_message_unref (reply);

    if (!res)
    {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

struct t_dbus_interface*
weechat_dbus_interfaces_peer_new (void)
{
    struct t_dbus_interface *iface;
    iface = weechat_dbus_interface_new (WEECHAT_DBUS_INTERFACES_PEER, false);
    if (NULL == iface)
    {
        return NULL;
    }

    struct t_dbus_method *m;
    m = weechat_dbus_method_new ("Ping",
                                 &weechat_dbus_interfaces_peer_ping,
                                 false, false);
    if (!m)
    {
        goto error;
    }

    int res = weechat_dbus_interface_add_method (iface, m);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }

    m = weechat_dbus_method_new ("GetMachineId",
                                 &weechat_dbus_interfaces_peer_get_machine_id,
                                 false, false);
    if (!m)
    {
        goto error;
    }

    res = weechat_dbus_method_add_arg(m, "machine_uuid",
                                      DBUS_TYPE_STRING_AS_STRING,
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

    return iface;

error:
    weechat_dbus_interface_unref (iface);
    return NULL;
}
