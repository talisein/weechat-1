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
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-interfaces-introspectable.h"
#include "dbus-object.h"
#include "dbus-interface.h"

static DBusHandlerResult
weechat_dbus_interfaces_introspectable_introspect (struct t_dbus_object *o,
                                                   DBusConnection *conn,
                                                   DBusMessage *msg)
{
    int rc;
    dbus_bool_t res;
    const int compression = 0;
    xmlTextWriterPtr writer;
    xmlBufferPtr buffer;

    buffer = xmlBufferCreate();
    writer = xmlNewTextWriterMemory(buffer, compression);

    rc = xmlTextWriterStartDocument (writer, NULL, "UTF-8", "no");
    if (-1 == rc)
    {
        goto error_xml;
    }
    rc = xmlTextWriterWriteDTD (writer,
                                BAD_CAST "node",
                                BAD_CAST DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER,
                                BAD_CAST DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER,
                                NULL);
    if (-1 == rc)
    {
        goto error_xml;
    }

    if (WEECHAT_RC_ERROR == weechat_dbus_object_introspect (o, writer, true))
    {
        goto error_xml;
    }

    rc = xmlTextWriterEndDocument (writer);
    if (-1 == rc)
    {
        goto error_xml;
    }

    const xmlChar *xmlstr = xmlBufferContent (buffer);
    DBusMessage *reply = dbus_message_new_method_return (msg);
    if (!reply)
    {
        xmlFreeTextWriter (writer);
        xmlBufferFree (buffer);
        goto error_msg;
    }

    res = dbus_message_append_args (reply,
                                    DBUS_TYPE_STRING, &xmlstr,
                                    DBUS_TYPE_INVALID);
    if (res)
    {
        res = dbus_connection_send (conn, reply, NULL);
    }


    xmlFreeTextWriter (writer);
    xmlBufferFree (buffer);

    if (!res)
    {
        goto error_msg;
    }

    return DBUS_HANDLER_RESULT_HANDLED;

error_xml:
    xmlFreeTextWriter (writer);
    xmlBufferFree (buffer);
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

error_msg:
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
}

struct t_dbus_interface*
weechat_dbus_interfaces_introspectable_new (void)
{
    struct t_dbus_interface *iface;
    iface = weechat_dbus_interface_new (DBUS_INTERFACE_INTROSPECTABLE,
                                        NULL,
                                        false);
    if (NULL == iface)
    {
        return NULL;
    }

    struct t_dbus_method *m;
    m = weechat_dbus_method_new ("Introspect",
                                 &weechat_dbus_interfaces_introspectable_introspect,
                                 false, false);
    if (!m)
    {
        goto error;
    }

    int res = weechat_dbus_method_add_arg(m, "xml_data",
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

