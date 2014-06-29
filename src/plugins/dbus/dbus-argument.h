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

#ifndef WEECHAT_DBUS_ARGUMENT_H
#define WEECHAT_DBUS_ARGUMENT_H 1

struct t_dbus_argument;

enum t_dbus_argument_direction
{
    WEECHAT_DBUS_ARGUMENT_DIRECTION_IN,
    WEECHAT_DBUS_ARGUMENT_DIRECTION_OUT
};

struct t_dbus_argument*
weechat_dbus_argument_list_get_tail (const struct t_dbus_argument *first);

int
weechat_dbus_argument_list_insert (struct t_dbus_argument *prev,
                                   struct t_dbus_argument *to_be_inserted);

void
weechat_dbus_argument_list_free_all (struct t_dbus_argument *first);

struct t_dbus_argument*
weechat_dbus_argument_new (const char *name, const char *type_signature,
                           enum t_dbus_argument_direction direction);

void
weechat_dbus_argument_free (struct t_dbus_argument *argument);

#endif /* WEECHAT_DBUS_ARGUMENT_H */
