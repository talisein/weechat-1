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

#ifndef WEECHAT_DBUS_INTERFACE_H
#define WEECHAT_DBUS_INTERFACE_H 1

#include <stdbool.h>
#include <dbus/dbus.h>
#include <libxml/xmlwriter.h>
#include "dbus-method.h"
#include "dbus-signal.h"
#include "dbus-property.h"

struct t_dbus_interface;
struct t_dbus_object;

struct t_dbus_interface *
weechat_dbus_interface_new (const char *name,
                            bool is_deprecated);

int
weechat_dbus_interface_add_method (struct t_dbus_interface *i,
                                   struct t_dbus_method *m);

int
weechat_dbus_interface_add_signal (struct t_dbus_interface *i,
                                   struct t_dbus_signal *s);

int
weechat_dbus_interface_add_property (struct t_dbus_interface *i,
                                     struct t_dbus_property *p);

const char *
weechat_dbus_interface_get_name (const struct t_dbus_interface *i);

void
weechat_dbus_interface_unref (struct t_dbus_interface *i);

void
weechat_dbus_interface_ref (const struct t_dbus_interface *i);

DBusHandlerResult
weechat_dbus_interface_handle_msg (const struct t_dbus_interface *i,
                                   struct t_dbus_object *o,
                                   DBusConnection *conn,
                                   DBusMessage *msg);

int
weechat_dbus_interface_introspect (struct t_dbus_interface *i,
                                   xmlTextWriterPtr writer);

#endif /* WEECHAT_DBUS_INTERFACE_H */
