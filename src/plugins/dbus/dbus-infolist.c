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
#include "dbus-infolist.h"
#include "../weechat-plugin.h"

#include "dbus-infolist.h"
#include "dbus.h"

int
weechat_dbus_infolist_append (DBusMessage *msg, struct t_infolist *infolist)
{
    if (!msg || !infolist)
        return 0;

    DBusMessageIter iter_top;
    DBusMessageIter iter_array;
    dbus_message_iter_init_append(msg, &iter_top);
    dbus_message_iter_open_container(&iter_top, DBUS_TYPE_ARRAY,
                                     DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                     DBUS_TYPE_STRING_AS_STRING
                                     DBUS_TYPE_VARIANT_AS_STRING
                                     DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                     &iter_array);

    while (0 != weechat_infolist_next(infolist))
    {
        const char *infolist_fields = weechat_infolist_fields(infolist);
        if (!infolist_fields)
            continue;

        char *fields = strdup(infolist_fields);
        char *p1, *p2;
        char *pair = strtok_r(fields, ",", &p1);

        for (; pair != NULL; pair = strtok_r(NULL, ",", &p1))
        {
            char *type = strtok_r(pair, ":", &p2);
            char *fieldname = strtok_r(NULL, ":", &p2);
            if (!type || !fieldname)
                continue;

            DBusMessageIter iter_dict;
            DBusMessageIter iter_variant;
            dbus_message_iter_open_container(&iter_array, DBUS_TYPE_DICT_ENTRY, NULL, &iter_dict);
            dbus_message_iter_append_basic(&iter_dict, DBUS_TYPE_STRING, &fieldname);


            if (strcmp(type, "i") == 0)
            {
                dbus_message_iter_open_container(&iter_dict, DBUS_TYPE_VARIANT, DBUS_TYPE_INT64_AS_STRING, &iter_variant);
                dbus_int64_t i = weechat_infolist_integer(infolist, fieldname);
                dbus_message_iter_append_basic(&iter_variant, DBUS_TYPE_INT64, &i);
                dbus_message_iter_close_container(&iter_dict, &iter_variant);
            }
            else if (strcmp(type, "s") == 0)
            {
                dbus_message_iter_open_container(&iter_dict, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &iter_variant);
                const char *str = weechat_infolist_string(infolist, fieldname);
                if (str)
                {
                    dbus_message_iter_append_basic(&iter_variant, DBUS_TYPE_STRING, &str);
                }
                else
                {
                    const char *empty = "";
                    dbus_message_iter_append_basic(&iter_variant, DBUS_TYPE_STRING, &empty);
                }
                dbus_message_iter_close_container(&iter_dict, &iter_variant);
            }
            else if (strcmp(type, "t") == 0)
            {
                dbus_message_iter_open_container(&iter_dict, DBUS_TYPE_VARIANT, DBUS_TYPE_INT64_AS_STRING, &iter_variant);
                dbus_int64_t t = weechat_infolist_time(infolist, fieldname);
                dbus_message_iter_append_basic(&iter_variant, DBUS_TYPE_INT64, &t);
                dbus_message_iter_close_container(&iter_dict, &iter_variant);
            }
            else
            {
                dbus_message_iter_open_container(&iter_dict, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &iter_variant);
                const char *str = "Not implemented";
                dbus_message_iter_append_basic(&iter_variant, DBUS_TYPE_STRING, &str);
                dbus_message_iter_close_container(&iter_dict, &iter_variant);
            }

            dbus_message_iter_close_container(&iter_array, &iter_dict);
        }

        free(fields);
    }
    dbus_message_iter_close_container(&iter_top, &iter_array);

    return 0;
}
