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

#ifndef WEECHAT_DBUS_INTERFACES_PEER_H
#define WEECHAT_DBUS_INTERFACES_PEER_H 1

#define WEECHAT_DBUS_INTERFACES_PEER "org.freedesktop.DBus.Peer"

struct t_dbus_interface;

struct t_dbus_interface*
weechat_dbus_interfaces_peer_new (void);

#endif /* WEECHAT_DBUS_INTERFACES_PEER_H */
