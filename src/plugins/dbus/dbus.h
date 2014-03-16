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

#ifndef __WEECHAT_DBUS_H
#define __WEECHAT_DBUS_H 1

#include <time.h>

#define weechat_plugin weechat_dbus_plugin
#define DBUS_PLUGIN_NAME "dbus"

struct t_dbus_ctx
{
    struct DBusConnection *conn;      /* Connection to Session Bus          */
    struct t_hook *dispatch;          /* Timeout hook for dbus dispatch     */ 
};

struct t_dbus_cmd
{
    int placeholder;
    /* command/process */
    int number;                        /* command number                    */

    struct t_dbus_cmd *prev_cmd;       /* link to previous command          */
    struct t_dbus_cmd *next_cmd;       /* link to next command              */
};

extern struct t_weechat_plugin *weechat_dbus_plugin;

#endif /* __WEECHAT_DBUS_H */
