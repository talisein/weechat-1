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

#include <dbus/dbus.h>
#include <stdlib.h>
#include <string.h>
#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-signal.h"
#include "dbus-argument.h"

struct t_dbus_signal
{
    struct t_dbus_argument *argument_head;
    char *name;
    bool is_deprecated;
};

struct t_dbus_signal *
weechat_dbus_signal_new (const char *name,
                         bool is_deprecated)
{
    if (!name)
    {
        return NULL;
    }

    DBusError err;
    dbus_error_init (&err);
    if (!dbus_validate_member (name, &err))
    {
        if (dbus_error_is_set (&err))
        {
            weechat_printf (NULL,
                            _("%s%s: Can't create signal %s: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            name, err.message);
            dbus_error_free (&err);  
        }
        return NULL;
    }

    struct t_dbus_signal *s = malloc (sizeof (struct t_dbus_signal));
    if (!s)
    {
        return NULL;
    }

    s->name = strdup(name);
    if (!s->name)
    {
        free (s);
        return NULL;
    }

    s->is_deprecated = is_deprecated;
    s->argument_head = NULL;

    return s;
}

int
weechat_dbus_signal_add_arg(struct t_dbus_signal *signal, const char *name,
                            const char *type_signature)
{
    if (!signal || !name || !type_signature)
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_dbus_argument *arg = weechat_dbus_argument_new(name, type_signature,
                                                            WEECHAT_DBUS_ARGUMENT_DIRECTION_OUT);
    if (!arg)
    {
        return WEECHAT_RC_ERROR;
    }

    if (!signal->argument_head)
    {
        signal->argument_head = arg;
        return WEECHAT_RC_OK;
    }

    struct t_dbus_argument *tail;
    tail = weechat_dbus_argument_list_get_tail(signal->argument_head);
    return weechat_dbus_argument_list_insert(tail, arg);
}

const char *
weechat_dbus_signal_get_name(const struct t_dbus_signal *signal)
{
    if (!signal)
    {
        return NULL;
    }

    return signal->name;
}

void
weechat_dbus_signal_free(struct t_dbus_signal *signal)
{
    if (!signal)
    {
        return;
    }

    if (signal->argument_head)
    {
        weechat_dbus_argument_list_free_all (signal->argument_head);
    }

    free (signal->name);
}
