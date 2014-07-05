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

#ifndef WEECHAT_DBUS_OBJECT_H
#define WEECHAT_DBUS_OBJECT_H 1

#include <stdbool.h>
#include <dbus/dbus.h>
#include <libxml/xmlwriter.h>
#include "dbus-interface.h"

struct t_dbus_object;

struct t_dbus_object *
weechat_dbus_object_new (struct t_dbus_object *parent, const char *path,
                         const void *obj);

int
weechat_dbus_object_add_interface (struct t_dbus_object *o,
                                   const struct t_dbus_interface *i);

int
weechat_dbus_object_add_child (struct t_dbus_object *parent,
                               const struct t_dbus_object *child);

void *
weechat_dbus_object_get_object (const struct t_dbus_object *obj);

void
weechat_dbus_object_unref (struct t_dbus_object *obj);

void
weechat_dbus_object_ref (const struct t_dbus_object *obj);

int
weechat_dbus_object_register (struct t_dbus_object *obj, DBusConnection *conn);

int
weechat_dbus_object_introspect (struct t_dbus_object *obj,
                                xmlTextWriterPtr writer,
                                bool is_root);

struct t_dbus_interface *
weechat_dbus_object_get_interface (struct t_dbus_object *obj,
                                   const char *interface_name);

#endif /* WEECHAT_DBUS_OBJECT_H */
