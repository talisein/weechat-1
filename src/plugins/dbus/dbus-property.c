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
#include "dbus-property.h"

struct t_dbus_property
{
    char *name;
    char *type_signature;
    enum t_dbus_property_access access;
    enum t_dbus_annotation_property_emits_changed_signal emits_changed;
    bool is_deprecated;
};

struct t_dbus_property *
weechat_dbus_property_new(const char *name,
                          const char *type_signature,
                          enum t_dbus_property_access access,
                          enum t_dbus_annotation_property_emits_changed_signal emits_changed,
                          bool is_deprecated)
{
    if (!name || !type_signature)
    {
        return NULL;
    }

    DBusError err;
    dbus_error_init (&err);

    if (!dbus_signature_validate_single (type_signature, &err))
    {
        if (dbus_error_is_set (&err))
        {
            weechat_printf (NULL,
                            _("%s%s: Can't create property %s with signature "
                              "%s: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            name, type_signature, err.message);
            dbus_error_free (&err);  
        }

        return NULL;
    }

    struct t_dbus_property *p = malloc (sizeof (struct t_dbus_property));
    if (!p)
    {
        return NULL;
    }

    p->name = strdup(name);
    if (!p->name)
    {
        free (p);
        return NULL;
    }

    p->type_signature = strdup(type_signature);
    if (!p->type_signature)
    {
        free (p->name);
        free (p);
        return NULL;
    }

    p->access = access;
    p->emits_changed = emits_changed;
    p->is_deprecated = is_deprecated;

    return p;
}

const char *
weechat_dbus_property_get_name(const struct t_dbus_property *property)
{
    if (!property)
    {
        return NULL;
    }

    return property->name;
}

void
weechat_dbus_property_free (struct t_dbus_property *property)
{
    if (!property)
    {
        return;
    }

    free (property->name);
    free (property->type_signature);
    free (property);
}

int
weechat_dbus_property_introspect (struct t_dbus_property *property,
                                  xmlTextWriterPtr writer)
{
    int rc;
    rc = xmlTextWriterStartElement (writer, BAD_CAST "property");
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "name",
                                      BAD_CAST property->name);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "type",
                                      BAD_CAST property->type_signature);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    const xmlChar str_rw[] = "readwrite";
    const xmlChar str_r[] = "read";
    const xmlChar str_w[] = "write";
    const xmlChar *str;

    switch (property->access)
    {
        case WEECHAT_DBUS_PROPERTY_ACCESS_READWRITE:
            str = &(str_rw[0]);
            break;
        case WEECHAT_DBUS_PROPERTY_ACCESS_READ:
            str = &(str_r[0]);
            break;
        case WEECHAT_DBUS_PROPERTY_ACCESS_WRITE:
            str = &(str_w[0]);
            break;
        default:
            return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "access",
                                      str);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterStartElement (writer, BAD_CAST "annotation");
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "name",
                                      BAD_CAST "org.freedesktop.DBus.Property.EmitsChangedSignal");
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    const xmlChar str_true[] = "true";
    const xmlChar str_invalidates[] = "invalidates";
    const xmlChar str_false[] = "false";
    switch (property->emits_changed)
    {
        case WEECHAT_DBUS_ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL_TRUE:
            str = &(str_true[0]);
            break;
        case WEECHAT_DBUS_ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL_INVALIDATES:
            str = &(str_invalidates[0]);
            break;
        case WEECHAT_DBUS_ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL_FALSE:
            str = &(str_false[0]);
            break;
        default:
            return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "value", str);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterEndElement (writer);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    rc = xmlTextWriterEndElement (writer);
    if (-1 == rc)
    {
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}
