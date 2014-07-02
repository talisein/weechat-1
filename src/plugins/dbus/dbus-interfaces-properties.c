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
#include "dbus-interface.h"


struct t_dbus_interface*
weechat_dbus_interfaces_properties_new (void)
{
    struct t_dbus_interface *iface;
    struct t_dbus_method *m;
    struct t_dbus_signal *s;
    int res;

    iface = weechat_dbus_interface_new (WEECHAT_DBUS_INTERFACES_PROPERTIES,
                                        false);
    if (NULL == iface)
    {
        return NULL;
    }

    /* org.freedesktop.DBus.Properties.Get */
    m = weechat_dbus_method_new ("Get", false, false);
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
                                      DBUS_TYPE_STRING_AS_STRING,
                                      WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_method_add_arg(m, "value",
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
    m = weechat_dbus_method_new ("Set", false, false);
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
                                      DBUS_TYPE_STRING_AS_STRING,
                                      WEECHAT_DBUS_ARGUMENT_DIRECTION_IN);
    if (WEECHAT_RC_ERROR == res)
    {
        weechat_dbus_method_free (m);
        goto error;
    }
    res = weechat_dbus_method_add_arg(m, "value",
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
    m = weechat_dbus_method_new ("GetAll", false, false);
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
