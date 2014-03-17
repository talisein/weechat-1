/*
 * dbus.c - Extension of WeeChat signals to DBus
 *
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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <dbus/dbus.h>

#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-mainloop.h"
#include "dbus-signal.h"

WEECHAT_PLUGIN_NAME(DBUS_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Extension of WeeChat signals to DBus"));
WEECHAT_PLUGIN_AUTHOR("Andrew Potter <agpotter@gmail.com>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_dbus_plugin = NULL;
struct t_dbus_ctx *ctx;

/*
 * Prints dbus infos in WeeChat log file (usually for crash dump).
 */

void
dbus_print_log ()
{
    /*
    struct t_dbus_cmd *ptr_dbus_cmd;

    for (ptr_dbus_cmd = dbus_cmds; ptr_dbus_cmd;
         ptr_dbus_cmd = ptr_dbus_cmd->next_cmd)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[dbus command (addr:0x%lx)]", ptr_dbus_cmd);
        weechat_log_printf ("  number. . . . . . . . . : %d",    ptr_dbus_cmd->number);
        weechat_log_printf ("  name. . . . . . . . . . : '%s'",  ptr_dbus_cmd->name);
        weechat_log_printf ("  hook. . . . . . . . . . : 0x%lx", ptr_dbus_cmd->hook);
        weechat_log_printf ("  command . . . . . . . . : '%s'",  ptr_dbus_cmd->command);
        weechat_log_printf ("  pid . . . . . . . . . . : %d",    ptr_dbus_cmd->pid);
        weechat_log_printf ("  start_time. . . . . . . : %ld",   ptr_dbus_cmd->start_time);
        weechat_log_printf ("  end_time. . . . . . . . : %ld",   ptr_dbus_cmd->end_time);
        weechat_log_printf ("  buffer_plugin . . . . . : '%s'",  ptr_dbus_cmd->buffer_plugin);
        weechat_log_printf ("  buffer_name . . . . . . : '%s'",  ptr_dbus_cmd->buffer_name);
        weechat_log_printf ("  output_to_buffer. . . . : %d",    ptr_dbus_cmd->output_to_buffer);
        weechat_log_printf ("  stdout_size . . . . . . : %d",    ptr_dbus_cmd->stdout_size);
        weechat_log_printf ("  stdout. . . . . . . . . : '%s'",  ptr_dbus_cmd->stdout);
        weechat_log_printf ("  stderr_size . . . . . . : %d",    ptr_dbus_cmd->stderr_size);
        weechat_log_printf ("  stderr. . . . . . . . . : '%s'",  ptr_dbus_cmd->stderr);
        weechat_log_printf ("  return_code . . . . . . : %d",    ptr_dbus_cmd->return_code);
        weechat_log_printf ("  prev_cmd. . . . . . . . : 0x%lx", ptr_dbus_cmd->prev_cmd);
        weechat_log_printf ("  next_cmd. . . . . . . . : 0x%lx", ptr_dbus_cmd->next_cmd);
    }
    */
}

/*
 * Callback for signal "debug_dump".
 */

int
dbus_debug_dump_cb (void *data, const char *signal, const char *type_data,
                    void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data
        || (weechat_strcasecmp ((char *)signal_data, DBUS_PLUGIN_NAME) == 0))
    {
        weechat_log_printf ("");
        weechat_log_printf ("***** \"%s\" plugin dump *****",
                            weechat_plugin->name);

        dbus_print_log ();

        weechat_log_printf ("");
        weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            weechat_plugin->name);
    }

    return WEECHAT_RC_OK;
}

const char*
weechat_dbus_get_session_bus_address(void)
{
    const char *var = getenv("DBUS_SESSION_BUS_ADDRESS");
    return var;
}

/*
 * Initializes dbus plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    weechat_hook_signal ("debug_dump", &dbus_debug_dump_cb, NULL);

    struct DBusError err;
    dbus_error_init(&err);

    ctx = calloc(1, sizeof(struct t_dbus_ctx));
    if (!ctx)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    const char *busaddr = weechat_dbus_get_session_bus_address();

    if (!busaddr) {
        weechat_printf (NULL,
                        _("%s%s: Error discovering DBus session address."),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        goto error;
    }

    ctx->conn = dbus_connection_open(busaddr, &err);
    if (dbus_error_is_set(&err)) {
        weechat_printf (NULL,
                        _("%s%s: Error connecting to session DBus: %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                        err.message);
        dbus_error_free(&err);
        goto error;
    }

    /* Set asynchronous handler functions */
    dbus_connection_set_dispatch_status_function (ctx->conn,
                                                  &weechat_dbus_set_dispatch,
                                                  ctx, NULL);

    if (!dbus_connection_set_watch_functions (ctx->conn,
                                              &weechat_dbus_add_watch,
                                              &weechat_dbus_remove_watch,
                                              &weechat_dbus_watch_toggled,
                                              ctx, NULL))
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        goto error;
    }
    
    if (!dbus_connection_set_timeout_functions(ctx->conn,
                                               &weechat_dbus_add_timeout,
                                               &weechat_dbus_remove_timeout,
                                               &weechat_dbus_timeout_toggled,
                                               ctx,
                                               NULL))
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        goto error;
    }

    /* Register on the session bus. Technically this is blocking... */
    dbus_bus_register(ctx->conn, &err);
    if (dbus_error_is_set(&err)) {
        weechat_printf (NULL,
                        _("%s%s: Error registering to session DBus: %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                        err.message);
        dbus_error_free(&err);
        goto error;
    }

    if (0 < weechat_dbus_hook_signals(ctx))
    {
        weechat_printf (NULL,
                        _("%s%s: Error hooking signals"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        goto error;
    }

    return WEECHAT_RC_OK;
error:
    if (ctx->conn)
    {
        dbus_connection_unref(ctx->conn);
        ctx->conn = NULL;
    }
    free(ctx);
    return WEECHAT_RC_ERROR;
}

/*
 * Ends dbus plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    if (ctx->sigctx) weechat_dbus_unhook_signals(ctx);
    if (ctx->conn) dbus_connection_unref(ctx->conn);
    free(ctx);

    return WEECHAT_RC_OK;
}
