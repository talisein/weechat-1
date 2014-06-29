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

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-argument.h"

struct t_dbus_argument
{
    struct t_dbus_argument *next;
    char *name;
    char *type_signature;
    enum t_dbus_argument_direction direction;
};

struct t_dbus_argument*
weechat_dbus_argument_list_get_tail(const struct t_dbus_argument *first)
{
    if (!first)
    {
        return NULL;
    }

    const struct t_dbus_argument *this = first;
    while (NULL != this->next)
    {
        this = this->next;
    }

    return (struct t_dbus_argument*)this;
}

int
weechat_dbus_argument_list_insert(struct t_dbus_argument *prev,
                                  struct t_dbus_argument *to_be_inserted)
{
    if (!prev || !to_be_inserted)
    {
        return WEECHAT_RC_ERROR;
    }

    if (NULL != to_be_inserted->next)
    {
        return WEECHAT_RC_ERROR;
    }

    to_be_inserted->next = prev->next;
    prev->next = to_be_inserted;

    return WEECHAT_RC_OK;
}

void
weechat_dbus_argument_list_free_all(struct t_dbus_argument *first)
{
    if (!first)
    {
        return;
    }

    struct t_dbus_argument *this = first;
    struct t_dbus_argument *next = first->next;
    while (NULL != next)
    {
        weechat_dbus_argument_free (this);
        this = next;
        next = next->next;
    }

    weechat_dbus_argument_free (this);
}

struct t_dbus_argument*
weechat_dbus_argument_new(const char *name, const char *type_signature,
                          enum t_dbus_argument_direction direction)
{
    if (!name || !type_signature)
    {
        return NULL;
    }

    DBusError err;
    dbus_error_init (&err);

    if (!dbus_signature_validate_single (type_signature, &err))
    {
        if (dbus_error_is_set (&err))
        {
            weechat_printf (NULL,
                            _("%s%s: Can't create argument %s with signature "
                              "%s: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            name, type_signature, err.message);
            dbus_error_free (&err);  
        }

        return NULL;
    }

    struct t_dbus_argument *argument = malloc (sizeof (struct t_dbus_argument));
    if (!argument)
    {
        return NULL;
    }

    argument->name = strdup (name);
    if (!argument->name)
    {
        free (argument);
        return NULL;
    }

    argument->type_signature = strdup (type_signature);
    if (!argument->type_signature)
    {
        free (argument->name);
        free (argument);
        return NULL;
    }

    argument->direction = direction;
    argument->next = NULL;

    return argument;
}

void
weechat_dbus_argument_free (struct t_dbus_argument *argument)
{
    if (!argument)
    {
        return;
    }

    free (argument->name);
    free (argument->type_signature);
    free (argument);
}
