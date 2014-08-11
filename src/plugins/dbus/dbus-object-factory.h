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

#include <stdbool.h>
#include <dbus/dbus.h>

struct t_dbus_object_factory;
struct t_gui_buffer;

struct t_dbus_object_factory *
weechat_dbus_object_factory_new (DBusConnection *conn);

void
weechat_dbus_object_factory_free (struct t_dbus_object_factory *factory);

int
weechat_dbus_object_factory_make_buffer (struct t_dbus_object_factory *factory,
                                         struct t_gui_buffer *buffer);

int
weechat_dbus_object_factory_make_all_buffers (struct t_dbus_object_factory *factory);
