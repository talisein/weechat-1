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

#ifndef WEECHAT_DBUS_METHOD_H
#define WEECHAT_DBUS_METHOD_H 1

#include <stdbool.h>
#include <dbus/dbus.h>
#include <libxml/xmlwriter.h>
#include "dbus-argument.h"

struct t_dbus_method;
struct t_dbus_object;

typedef DBusHandlerResult (*t_dbus_method_handler)(struct t_dbus_object *o,
                                                   DBusConnection *conn,
                                                   DBusMessage *msg);

struct t_dbus_method *
weechat_dbus_method_new(const char *name,
                        t_dbus_method_handler handler,
                        bool is_deprecated,
                        bool is_no_reply);

int
weechat_dbus_method_add_arg(struct t_dbus_method *method,
                            const char *name,
                            const char *type_signature,
                            enum t_dbus_argument_direction);

const char *
weechat_dbus_method_get_name(const struct t_dbus_method *method);

void
weechat_dbus_method_free(struct t_dbus_method *method);

DBusHandlerResult
weechat_dbus_method_handle_msg (const struct t_dbus_method *method,
                                struct t_dbus_object *o,
                                DBusConnection *conn,
                                DBusMessage *msg);

int
weechat_dbus_method_introspect (struct t_dbus_method *method,
                                xmlTextWriterPtr writer);

#endif /* WEECHAT_DBUS_METHOD_H */
