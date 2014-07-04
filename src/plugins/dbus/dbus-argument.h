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

#include <stdbool.h>
#include <libxml/xmlwriter.h>

struct t_dbus_argument;

enum t_dbus_argument_direction
{
    WEECHAT_DBUS_ARGUMENT_DIRECTION_IN,
    WEECHAT_DBUS_ARGUMENT_DIRECTION_OUT
};

struct t_dbus_argument*
weechat_dbus_argument_new (const char *name, const char *type_signature,
                           enum t_dbus_argument_direction direction);

void
weechat_dbus_argument_free (struct t_dbus_argument *argument);

struct t_dbus_argument_list;

struct t_dbus_argument_list *
weechat_dbus_argument_list_new (void);

int
weechat_dbus_argument_list_append (struct t_dbus_argument_list *list,
                                   struct t_dbus_argument *arg);

void
weechat_dbus_argument_list_free (struct t_dbus_argument_list *list);

int
weechat_dbus_argument_list_introspect (struct t_dbus_argument_list *list,
                                       xmlTextWriterPtr writer,
                                       bool is_signal);
#endif /* WEECHAT_DBUS_ARGUMENT_H */
