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

struct t_dbus_argument_list
{
    struct t_dbus_argument *head;
    struct t_dbus_argument *tail;
};

struct t_dbus_argument
{
    struct t_dbus_argument *next;
    char *name;
    char *type_signature;
    enum t_dbus_argument_direction direction;
};


struct t_dbus_argument_list *
weechat_dbus_argument_list_new (void)
{
    return calloc(1, sizeof(struct t_dbus_argument_list));
}

int
weechat_dbus_argument_list_append (struct t_dbus_argument_list *list,
                                   struct t_dbus_argument *arg)
{
    if (!list || !arg)
    {
        return WEECHAT_RC_ERROR;
    }

    if (NULL != arg->next)
    {
        return WEECHAT_RC_ERROR;
    }

    if (list->head)
    {
        list->tail->next = arg;
        list->tail = arg;
    }
    else
    {
        list->head = arg;
        list->tail = arg;
    }

    return WEECHAT_RC_OK;
}

static void
weechat_dbus_argument_list_free_all(struct t_dbus_argument_list *list)
{
    if (!list)
    {
        return;
    }

    if (!list->head)
    {
        return;
    }

    struct t_dbus_argument *this = list->head;
    struct t_dbus_argument *next = this->next;
    while (NULL != next)
    {
        weechat_dbus_argument_free (this);
        this = next;
        next = next->next;
    }

    weechat_dbus_argument_free (this);
}

void
weechat_dbus_argument_list_free(struct t_dbus_argument_list *list)
{
    if (!list)
    {
        return;
    }

    weechat_dbus_argument_list_free_all (list);
    free (list);
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

int
weechat_dbus_argument_list_introspect (struct t_dbus_argument_list *list,
                                       xmlTextWriterPtr writer,
                                       bool is_signal)
{
    int rc;
    struct t_dbus_argument *arg;

    for (arg = list->head; arg; arg = arg->next)
    {
        rc = xmlTextWriterStartElement (writer, BAD_CAST "arg");
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }

        rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "name",
                                          BAD_CAST arg->name);
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }

        rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "type",
                                          BAD_CAST arg->type_signature);
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }

        if (!is_signal)
        {
            const xmlChar in_str[] = "in";
            const xmlChar out_str[] = "out";
            const xmlChar *str;
            switch (arg->direction)
            {
                case WEECHAT_DBUS_ARGUMENT_DIRECTION_IN:
                    str = &(in_str[0]);
                    break;
                case WEECHAT_DBUS_ARGUMENT_DIRECTION_OUT:
                    str = &(out_str[0]);
                    break;
                default:
                    return WEECHAT_RC_ERROR;
            }

            rc = xmlTextWriterWriteAttribute (writer, BAD_CAST "direction",
                                              str);
            if (-1 == rc)
            {
                return WEECHAT_RC_ERROR;
            }
        }

        rc = xmlTextWriterEndElement (writer);
        if (-1 == rc)
        {
            return WEECHAT_RC_ERROR;
        }
    }

    return WEECHAT_RC_OK;
}
