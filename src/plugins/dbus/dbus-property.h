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

#ifndef WEECHAT_DBUS_PROPERTIES_H
#define WEECHAT_DBUS_PROPERTIES_H 1

#include <stdbool.h>
#include <dbus/dbus.h>
#include <libxml/xmlwriter.h>

struct t_dbus_property;
struct t_dbus_interface;
struct t_dbus_object;

enum t_dbus_property_access
{
    WEECHAT_DBUS_PROPERTY_ACCESS_READWRITE,
    WEECHAT_DBUS_PROPERTY_ACCESS_READ,
    WEECHAT_DBUS_PROPERTY_ACCESS_WRITE
};

enum t_dbus_annotation_property_emits_changed_signal
{
    WEECHAT_DBUS_ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL_TRUE,
    WEECHAT_DBUS_ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL_INVALIDATES,
    WEECHAT_DBUS_ANNOTATION_PROPERTY_EMITS_CHANGED_SIGNAL_FALSE
};

typedef DBusHandlerResult (*property_getter)(struct t_dbus_property *prop,
                                             struct t_dbus_object *o,
                                             struct t_dbus_interface *i,
                                             DBusConnection *conn,
                                             DBusMessage *msg);

typedef DBusHandlerResult (*property_setter)(struct t_dbus_property *prop,
                                             struct t_dbus_object *o,
                                             struct t_dbus_interface *i,
                                             DBusConnection *conn,
                                             DBusMessage *msg,
                                             DBusMessageIter *iter);

struct t_dbus_property *
weechat_dbus_property_new(const char *name,
                          const char *type_signature,
                          property_getter getter,
                          property_setter setter,
                          enum t_dbus_property_access access,
                          enum t_dbus_annotation_property_emits_changed_signal emits_changed,
                          bool is_deprecated);

const char *
weechat_dbus_property_get_name(const struct t_dbus_property *property);

void
weechat_dbus_property_free (struct t_dbus_property *property);

int
weechat_dbus_property_introspect (struct t_dbus_property *property,
                                  xmlTextWriterPtr writer);

DBusHandlerResult
weechat_dbus_property_get (struct t_dbus_property *property,
                           struct t_dbus_object *o,
                           struct t_dbus_interface *i,
                           DBusConnection *conn,
                           DBusMessage *reply);

DBusHandlerResult
weechat_dbus_property_set (struct t_dbus_property *property,
                           struct t_dbus_object *o,
                           struct t_dbus_interface *i,
                           DBusConnection *conn,
                           DBusMessage *reply,
                           DBusMessageIter *iter);

#endif /* WEECHAT_DBUS_PROPERTIES_H */
