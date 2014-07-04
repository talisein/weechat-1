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
#include "dbus-interfaces-object-manager.h"
#include "dbus-interface.h"

static DBusHandlerResult
weechat_dbus_interfaces_object_manager_get_managed_objects (struct t_dbus_object *o,
                                                            DBusConnection *conn,
                                                            DBusMessage *msg)
{
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

struct t_dbus_interface*
weechat_dbus_interfaces_object_manager_new (void)
{
    struct t_dbus_interface *iface;
    struct t_dbus_method *m;
    struct t_dbus_signal *s;
    int res;

    iface = weechat_dbus_interface_new (WEECHAT_DBUS_INTERFACES_OBJECT_MANAGER,
                                        false);
    if (NULL == iface)
    {
        return NULL;
    }

    /* org.freedesktop.DBus.ObjectManager.GetManagedObjects */
    m = weechat_dbus_method_new ("GetManagedObjects",
                                 &weechat_dbus_interfaces_object_manager_get_managed_objects,
                                 false, false);
    if (!m)
    {
        goto error;
    }
    /* DICT<OBJPATH,DICT<STRING,DICT<STRING,VARIANT>>> */
    res = weechat_dbus_method_add_arg(m, "objpath_interfaces_and_properties",
                                      DBUS_TYPE_ARRAY_AS_STRING
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                      DBUS_TYPE_OBJECT_PATH_AS_STRING
                                      DBUS_TYPE_ARRAY_AS_STRING
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                      DBUS_TYPE_STRING_AS_STRING
                                      DBUS_TYPE_ARRAY_AS_STRING
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                      DBUS_TYPE_STRING_AS_STRING
                                      DBUS_TYPE_VARIANT_AS_STRING
                                      DBUS_DICT_ENTRY_END_CHAR_AS_STRING
                                      DBUS_DICT_ENTRY_END_CHAR_AS_STRING
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

    /* org.freedesktop.DBus.ObjectManager.InterfacesAdded */
    s = weechat_dbus_signal_new ("InterfacesAdded", false);
    if (!s)
    {
        goto error;
    }
    res = weechat_dbus_signal_add_arg (s, "object_path",
                                       DBUS_TYPE_OBJECT_PATH_AS_STRING);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_signal_free (s);
        goto error;
    }
    /* DICT<STRING,DICT<STRING,VARIANT>> */
    res = weechat_dbus_signal_add_arg (s, "interfaces_and_properties",
                                       DBUS_TYPE_ARRAY_AS_STRING
                                       DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                       DBUS_TYPE_STRING_AS_STRING
                                       DBUS_TYPE_ARRAY_AS_STRING
                                       DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                       DBUS_TYPE_STRING_AS_STRING
                                       DBUS_TYPE_VARIANT_AS_STRING
                                       DBUS_DICT_ENTRY_END_CHAR_AS_STRING
                                       DBUS_DICT_ENTRY_END_CHAR_AS_STRING);
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

    /* org.freedesktop.DBus.ObjectManager.InterfacesRemoved */
    s = weechat_dbus_signal_new ("InterfacesRemoved", false);
    if (!s)
    {
        goto error;
    }
    res = weechat_dbus_signal_add_arg (s, "object_path",
                                       DBUS_TYPE_OBJECT_PATH_AS_STRING);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_signal_free (s);
        goto error;
    }
    res = weechat_dbus_signal_add_arg (s, "interfaces",
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
