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

#include <string.h>
#include <dbus/dbus.h>
#include "../weechat-plugin.h"

#include "dbus-methods-core.h"
#include "dbus-strings.h"
#include "dbus.h"

static DBusHandlerResult
message_handler_core_get_buffers(DBusConnection *connection, DBusMessage *message, void *user_data)
{
    (void) user_data;

    DBusError err;
    dbus_error_init (&err);

    struct t_hdata *ptr_hdata;
    struct t_gui_buffer *ptr_buffer;
    
    DBusMessage *reply = dbus_message_new_method_return (message);
    if (!reply) return DBUS_HANDLER_RESULT_NEED_MEMORY;
    
    ptr_hdata = weechat_hdata_get("buffer");
    int full_name_offset = weechat_hdata_get_var_offset(ptr_hdata, "full_name");

    DBusMessageIter iter_top;
    DBusMessageIter iter_array;
    dbus_message_iter_init_append (reply, &iter_top);
    dbus_message_iter_open_container (&iter_top, DBUS_TYPE_ARRAY,
                                      DBUS_TYPE_STRING_AS_STRING,
                                      &iter_array);

    ptr_buffer = weechat_hdata_get_list (ptr_hdata, "gui_buffers");
    while (ptr_buffer)
    {
        const char *name;
        name = *(const char**)weechat_hdata_get_var_at_offset (ptr_hdata, ptr_buffer, full_name_offset);
        if (name == NULL)
            name = "";
        dbus_message_iter_append_basic(&iter_array, DBUS_TYPE_STRING, &name);
        ptr_buffer = weechat_hdata_move (ptr_hdata, ptr_buffer, 1);
    }
    dbus_message_iter_close_container(&iter_top, &iter_array);
    dbus_connection_send (connection, reply, NULL);
    dbus_message_unref (reply);

    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
message_handler_core_command(DBusConnection *connection, DBusMessage *message, void *user_data)
{
    (void) user_data;

    DBusError err;
    dbus_error_init (&err);
    const char *name = NULL, *command;

    dbus_message_get_args (message, &err,
                           DBUS_TYPE_STRING, &name,
                           DBUS_TYPE_STRING, &command,
                           DBUS_TYPE_INVALID);
    if (dbus_error_is_set (&err))
    {
        DBusMessage *reply;
        reply = dbus_message_new_error_printf (message, DBUS_ERROR_INVALID_ARGS,
                                               "Method %s requires signature ss. "
                                               "The 1st can be \"\""
                                               "to specify the core buffer.",
                                               WEECHAT_DBUS_MEMBER_CORE_COMMAND);
        dbus_error_free (&err);
        if (!reply) return DBUS_HANDLER_RESULT_NEED_MEMORY;
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    else
    {
        struct t_gui_buffer *buf = NULL;
        if (name && name[0])
        {
            buf = weechat_buffer_search("==", name);
        }
        else
        {
            buf = weechat_buffer_search_main ();
        }
        if (buf == NULL)
        {
            DBusMessage *reply;
            reply = dbus_message_new_error_printf (message, DBUS_ERROR_INVALID_ARGS,
                                                   "No buffer found");
            if (!reply) return DBUS_HANDLER_RESULT_NEED_MEMORY;
            dbus_connection_send (connection, reply, NULL);
            dbus_message_unref (reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        else
        {
            DBusMessage *reply = dbus_message_new_method_return (message);
            if (!reply) return DBUS_HANDLER_RESULT_NEED_MEMORY;
            weechat_printf (buf,
                            _("%s: Executing command '%s'"),
                            DBUS_PLUGIN_NAME, command);
            weechat_command(buf, command);
            dbus_connection_send (connection, reply, NULL);
            dbus_message_unref (reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }
}

static DBusHandlerResult
message_handler_core_info_get(DBusConnection *connection, DBusMessage *message, void *user_data)
{
    (void) user_data;

    DBusError err;
    dbus_error_init (&err);
    const char *info_name = NULL, *arguments = NULL;

    dbus_message_get_args (message, &err,
                           DBUS_TYPE_STRING, &info_name,
                           DBUS_TYPE_STRING, &arguments,
                           DBUS_TYPE_INVALID);
    if (dbus_error_is_set (&err))
    {
        DBusMessage *reply;
        reply = dbus_message_new_error_printf (message, DBUS_ERROR_INVALID_ARGS,
                                               "Method %s requires signature ss. "
                                               "The second string can be \"\".",
                                               WEECHAT_DBUS_MEMBER_CORE_INFOGET);
        dbus_error_free (&err);
        if (!reply) return DBUS_HANDLER_RESULT_NEED_MEMORY;
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    else
    {
        const char *ret = weechat_info_get (info_name, arguments);
        if (ret)
        {
            DBusMessage *reply = dbus_message_new_method_return (message);
            if (!reply) return DBUS_HANDLER_RESULT_NEED_MEMORY;
            dbus_message_append_args (reply, DBUS_TYPE_STRING, &ret, DBUS_TYPE_INVALID);
            dbus_connection_send (connection, reply, NULL);
            dbus_message_unref (reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        else
        {
            DBusMessage *reply;
            reply = dbus_message_new_error_printf (message, DBUS_ERROR_INVALID_ARGS,
                                                   "info_name '%s' or argument is invalid",
                                                   info_name);
            if (!reply) return DBUS_HANDLER_RESULT_NEED_MEMORY;
            dbus_connection_send (connection, reply, NULL);
            dbus_message_unref (reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }
}

static void
unregister_handler_core(DBusConnection *connection, void *user_data)
{
    (void) connection;
    (void) user_data;
    weechat_printf (NULL,
                    "%s: In handler for unregister!"
                    DBUS_PLUGIN_NAME);
}

static DBusHandlerResult
message_handler_core(DBusConnection *connection, DBusMessage *message, void *user_data)
{
    (void) user_data;
    if (!dbus_message_has_interface (message, WEECHAT_DBUS_IFACE_CORE))
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (dbus_message_has_member (message, WEECHAT_DBUS_MEMBER_CORE_INFOGET))
    {
        return message_handler_core_info_get(connection, message, user_data);
    }
    if (dbus_message_has_member (message, WEECHAT_DBUS_MEMBER_CORE_COMMAND))
    {
        return message_handler_core_command(connection, message, user_data);
    }
    if (dbus_message_has_member (message, WEECHAT_DBUS_MEMBER_CORE_GET_BUFFERS))
    {
        return message_handler_core_get_buffers(connection, message, user_data);
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int
weechat_dbus_methods_core_register(struct t_dbus_ctx *ctx)
{
    DBusObjectPathVTable opt;
    DBusError err;
    dbus_error_init (&err);
    opt.unregister_function = unregister_handler_core;
    opt.message_function = message_handler_core;

    dbus_connection_try_register_fallback (ctx->conn, WEECHAT_DBUS_OBJECT_CORE, &opt, ctx, &err);
    if (dbus_error_is_set (&err))
    {
        goto error;
    }


    return 0;
error:
    weechat_printf (NULL,
                    _("%s%s: Error registering %s: %s"),
                    weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                    WEECHAT_DBUS_OBJECT_CORE, err.message);
    dbus_error_free (&err);
    return -1;
}
