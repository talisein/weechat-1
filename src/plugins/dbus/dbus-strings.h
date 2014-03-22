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

#ifndef WEECHAT_DBUS_STRINGS_H
#define WEECHAT_DBUS_STRINGS_H 1

extern const char WEECHAT_DBUS_NAME[];

/* Signals */
extern const char WEECHAT_DBUS_OBJECT_SIGNAL[];

extern const char WEECHAT_DBUS_IFACE_SIGNAL_IRC[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_IN[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_OUT[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_CTCP[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_DCC[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_PV[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_CONNECTING[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_CONNECTED[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_DISCONNECTED[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_JOIN[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_QUIT[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_AWAY[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_STILL_AWAY[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_BACK[];

extern const char WEECHAT_DBUS_IFACE_SIGNAL_CORE[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_DAY_CHANGED[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PLUGIN_LOADED[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PLUGIN_UNLOADED[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_QUIT[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_UPGRADE[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_HIGHLIGHT[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PV[];

extern const char WEECHAT_DBUS_IFACE_SIGNAL_XFER[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ADD[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_SEND_READY[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ACCEPT_RESUME[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_SEND_ACCEPT_RESUME[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_START_RESUME[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_RESUME_READY[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ENDED[];

extern const char WEECHAT_DBUS_IFACE_SIGNAL_GUILE[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_LUA[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_PERL[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_PYTHON[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_RUBY[];
extern const char WEECHAT_DBUS_IFACE_SIGNAL_TCL[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_LOADED[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_UNLOADED[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_INSTALLED[];
extern const char WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_REMOVED[];

/* Methods */
extern const char WEECHAT_DBUS_OBJECT_CORE[];
extern const char WEECHAT_DBUS_IFACE_CORE[];
extern const char WEECHAT_DBUS_MEMBER_CORE_INFOGET[];

#endif /* WEECHAT_STRINGS_H */
