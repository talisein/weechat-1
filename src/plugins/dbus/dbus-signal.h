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

#ifndef __WEECHAT_DBUS_SIGNAL_H
#define __WEECHAT_DBUS_SIGNAL_H 1


struct t_dbus_signal_ctx;
struct t_dbus_ctx;

extern const char WEECHAT_DBUS_OBJECT_SIGNAL[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_IRC[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_CORE[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_XFER[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_GUILE[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_LUA[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_PERL[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_PYTHON[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_RUBY[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_TCL[];

int
weechat_dbus_hook_signals(struct t_dbus_ctx *ctx);

void
weechat_dbus_unhook_signals(struct t_dbus_ctx *ctx);

#endif /* __WEECHAT_DBUS_SIGNAL_H */
