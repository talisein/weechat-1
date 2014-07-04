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
#include <string.h>
#include <dbus/dbus.h>
#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-method.h"
#include "dbus-argument.h"

struct t_dbus_method
{
    struct t_dbus_argument_list *list;
    char *name;
    t_dbus_method_handler handler;
    bool is_deprecated;
    bool is_no_reply;
};

struct t_dbus_method *
weechat_dbus_method_new (const char *name,
                         t_dbus_method_handler handler,
                         bool is_deprecated,
                         bool is_no_reply)
{
    if (!name)
    {
        return NULL;
    }

    DBusError err;
    dbus_error_init (&err);
    if (!dbus_validate_member (name, &err))
    {
        if (dbus_error_is_set (&err))
        {
            weechat_printf (NULL,
                            _("%s%s: Can't create method %s: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            name, err.message);
            dbus_error_free (&err);  
        }
        return NULL;
    }

    struct t_dbus_method *m = malloc (sizeof (struct t_dbus_method));
    if (!m)
    {
        return NULL;
    }

    m->name = strdup (name);
    if (!m->name)
    {
        free (m);
        return NULL;
    }

    m->list = weechat_dbus_argument_list_new();
    if (!m->list)
    {
        free (m->name);
        free (m);
        return NULL;
    }

    m->handler = handler;
    m->is_deprecated = is_deprecated;
    m->is_no_reply = is_no_reply;
    
    return m;
}

int
weechat_dbus_method_add_arg (struct t_dbus_method *method,
                             const char *name,
                             const char *type_signature,
                             enum t_dbus_argument_direction direction)
{
    if (!method || !name || !type_signature)
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_dbus_argument *arg = weechat_dbus_argument_new (name, type_signature,
                                                             direction);
    if (!arg)
    {
        return WEECHAT_RC_ERROR;
    }

    return weechat_dbus_argument_list_append(method->list, arg);
}

const char *
weechat_dbus_method_get_name (const struct t_dbus_method *method)
{
    if (!method)
    {
        return NULL;
    }

    return method->name;
}

void
weechat_dbus_method_free (struct t_dbus_method *method)
{
    if (!method)
    {
        return;
    }

    weechat_dbus_argument_list_free (method->list);
    free (method->name);
    free (method);
}

DBusHandlerResult
weechat_dbus_method_handle_msg (const struct t_dbus_method *method,
                                struct t_dbus_object *o,
                                DBusConnection *conn,
                                DBusMessage *msg)
{
    return method->handler(o, conn, msg);
}

int
weechat_dbus_method_introspect (struct t_dbus_method *method,
                                xmlTextWriterPtr writer)
{
    int rc;
    int res;
    rc = xmlTextWriterStartElement (writer, BAD_CAST "method");
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "name",
                                      BAD_CAST method->name);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    res = weechat_dbus_argument_list_introspect (method->list, writer, false);
    if (WEECHAT_RC_ERROR == res)
    {
        return WEECHAT_RC_ERROR;
    }

    if (method->is_no_reply)
    {
        rc = xmlTextWriterStartElement (writer, BAD_CAST "annotation");
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }

        rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "name",
                                          BAD_CAST "org.freedesktop.DBus.Method.NoReply");
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

    if (method->is_deprecated)
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
