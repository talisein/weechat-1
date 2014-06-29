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
#include "dbus-object.h"

struct t_dbus_object
{
    const struct t_dbus_object *parent;
    struct t_hashtable         *interface_ht;
    struct t_hashtable         *children_ht;
    char                       *name;
    const void                 *obj;
    size_t                      ref_cnt;
};

static void
weechat_dbus_object_interfaces_free (struct t_hashtable *hashtable, const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_interface_unref ( (struct t_dbus_interface*)value);
}

static void
weechat_dbus_object_objects_free (struct t_hashtable *hashtable, const void *key, void *value)
{
    (void) hashtable;
    (void) key;

    weechat_dbus_object_unref ( (struct t_dbus_object*)value);
}

struct t_dbus_object *
weechat_dbus_object_new (struct t_dbus_object *parent,
                         const char *name,
                         const void *obj)
{
    if (!name) return NULL;

    struct t_dbus_object *o = malloc (sizeof (struct t_dbus_object));
    if (!o) return NULL;

    o->interface_ht = weechat_hashtable_new (8, WEECHAT_HASHTABLE_STRING,
                                             WEECHAT_HASHTABLE_POINTER,
                                             NULL, NULL);
    if (!o->interface_ht) {
        free (o);
        return NULL;
    }
    o->children_ht = weechat_hashtable_new (8, WEECHAT_HASHTABLE_POINTER,
                                            WEECHAT_HASHTABLE_POINTER,
                                            NULL, NULL);
    if (!o->children_ht) {
        weechat_hashtable_free (o->interface_ht);
        free (o);
        return NULL;
    }

    o->name = strdup (name);
    if (!o->name)
    {
        weechat_hashtable_free (o->children_ht);
        weechat_hashtable_free (o->interface_ht);
        free (o);
        return NULL;
    }

    weechat_hashtable_set_pointer (o->interface_ht, "callback_free_value",
                                   &weechat_dbus_object_interfaces_free);

    weechat_hashtable_set_pointer (o->children_ht, "callback_free_value",
                                   &weechat_dbus_object_objects_free);

    o->obj = obj;
    o->parent = parent;
    o->ref_cnt = 1;

    return o;
}

int
weechat_dbus_object_add_interface (struct t_dbus_object *o,
                                   const struct t_dbus_interface *i)
{
    const char *iname = weechat_dbus_interface_get_name (i);

    if (weechat_hashtable_has_key (o->interface_ht, iname))
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_hashtable_item *item = weechat_hashtable_set (o->interface_ht, iname, i);
    if (!item)
    {
        return WEECHAT_RC_ERROR;
    }

    weechat_dbus_interface_ref (i);

    return WEECHAT_RC_OK;
}

int
weechat_dbus_object_add_child (struct t_dbus_object *parent,
                               const struct t_dbus_object *child)
{
    if (!parent || !child)
    {
        return WEECHAT_RC_ERROR;
    }

    if (weechat_hashtable_has_key (parent->children_ht, child))
    {
        return WEECHAT_RC_ERROR;
    }

    struct t_hashtable_item *item = weechat_hashtable_set (parent->children_ht, child, child);
    if (!item)
    {
        return WEECHAT_RC_ERROR;
    }

    weechat_dbus_object_ref (child);

    return WEECHAT_RC_OK;
}

void
weechat_dbus_object_unref (struct t_dbus_object *o)
{
    if (!o)
    {
        return;
    }

    --o->ref_cnt;

    if (0 == o->ref_cnt)
    {
        weechat_hashtable_free (o->interface_ht);
        weechat_hashtable_free (o->children_ht);
        free (o->name);
    }
}

void
weechat_dbus_object_ref (const struct t_dbus_object *o)
{
    if (!o)
    {
        return;
    }

    /* ref_cnt is mutable */
    size_t *cnt = (size_t*)&o->ref_cnt;

    ++(*cnt);
}
