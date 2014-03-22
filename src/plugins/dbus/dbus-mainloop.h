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

#ifndef WEECHAT_DBUS_MAINLOOP_H
#define WEECHAT_DBUS_MAINLOOP_H 1

#include <dbus/dbus.h>

/* Functions to facilitate WeeChat mainloop <-> libdbus interaction */

dbus_bool_t weechat_dbus_add_watch       (DBusWatch *watch, void *data);
void        weechat_dbus_remove_watch    (DBusWatch *watch, void *data);
void        weechat_dbus_watch_toggled   (DBusWatch *watch, void *data);
dbus_bool_t weechat_dbus_add_timeout     (DBusTimeout *timeout, void *data);
void        weechat_dbus_remove_timeout  (DBusTimeout *timeout, void *data);
void        weechat_dbus_timeout_toggled (DBusTimeout *timeout, void *data);
void        weechat_dbus_set_dispatch    (DBusConnection *connection,
                                          DBusDispatchStatus new_status,
                                          void *data);

#endif /* WEECHAT_DBUS_MAINLOOP_H */
