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
#include "dbus-interfaces-buffer.h"
#include "dbus-interface.h"

static DBusHandlerResult
weechat_dbus_interfaces_buffer_command (struct t_dbus_object *o,
                                        DBusConnection *conn,
                                        DBusMessage *msg)
{
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


struct t_dbus_interface*
weechat_dbus_interfaces_buffer_new (void)
{
    struct t_dbus_interface *iface;
    struct t_dbus_method *m;
    int res;

    iface = weechat_dbus_interface_new (WEECHAT_DBUS_INTERFACES_BUFFER, false);
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

