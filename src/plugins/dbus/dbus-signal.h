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

#ifndef WEECHAT_DBUS_SIGNAL_H
#define WEECHAT_DBUS_SIGNAL_H 1

#include <stdbool.h>
#include <libxml/xmlwriter.h>

struct t_dbus_signal;

struct t_dbus_signal *
weechat_dbus_signal_new(const char *name,
                        bool is_deprecated);

int
weechat_dbus_signal_add_arg(struct t_dbus_signal *signal,
                            const char *name,
                            const char *type_signature);

const char *
weechat_dbus_signal_get_name(const struct t_dbus_signal *signal);

void
weechat_dbus_signal_free (struct t_dbus_signal *signal);

int
weechat_dbus_signal_introspect (struct t_dbus_signal *signal,
                                xmlTextWriterPtr writer);

#endif /* WEECHAT_DBUS_SIGNAL_H */
