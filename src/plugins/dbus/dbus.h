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

#ifndef WEECHAT_DBUS_H
#define WEECHAT_DBUS_H 1

#define weechat_plugin weechat_dbus_plugin
#define DBUS_PLUGIN_NAME "dbus"

#include <dbus/dbus.h>

struct t_dbus_mainloop_ctx;
struct t_dbus_signal_ctx;
struct t_dbus_object_factory;

struct t_dbus_ctx
{
    DBusConnection *conn;             /* Connection to Session Bus          */
    struct t_dbus_mainloop_ctx *main; /* Context for main loop hooks        */
    struct t_dbus_signal_ctx *sigctx; /* Context for hooked signals         */
    struct t_dbus_object_factory *factory; /* Makes objects as they appear  */
};

extern struct t_weechat_plugin *weechat_dbus_plugin;

int
weechat_dbus_hook_signals(struct t_dbus_ctx *ctx);

void
weechat_dbus_unhook_signals(struct t_dbus_ctx *ctx);


#endif /* WEECHAT_DBUS_H */
