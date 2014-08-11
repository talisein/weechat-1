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
#include "dbus-infolist.h"
#include "dbus-strings.h"
#include "dbus-methods-core.h"
#include "dbus-object-factory.h"

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

static const char*
weechat_dbus_get_session_bus_address(void)
{
    const char *var = getenv ("DBUS_SESSION_BUS_ADDRESS");
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
    int rc;

    weechat_dbus_plugin = plugin;

    weechat_hook_signal ("debug_dump", &dbus_debug_dump_cb, NULL);

    DBusError err;
    dbus_error_init (&err);

    ctx = calloc (1, sizeof (struct t_dbus_ctx));
    if (!ctx)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    const char *busaddr = weechat_dbus_get_session_bus_address ();
    if (!busaddr)
    {
        weechat_printf (NULL,
                        _("%s%s: Error discovering DBus session address."),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        goto error;
    }

    ctx->conn = dbus_connection_open (busaddr, &err);
    if (dbus_error_is_set (&err))
    {
        weechat_printf (NULL,
                        _("%s%s: Error connecting to session DBus: %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                        err.message);
        dbus_error_free (&err);
        goto error;
    }

    if (0 > weechat_dbus_hook_mainloop (ctx))
        goto error;

    /* Register on the session bus. Technically this is blocking... */
    dbus_bus_register (ctx->conn, &err);
    if (dbus_error_is_set (&err))
    {
        weechat_printf (NULL,
                        _("%s%s: Error registering to session DBus: %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                        err.message);
        dbus_error_free (&err);
        goto error;
    }

    /* Hook signals. This is old code that will be removed. */
    if (0 < weechat_dbus_hook_signals (ctx))
    {
        weechat_printf (NULL,
                        _("%s%s: Error hooking signals"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        goto error;
    }

    /* Register name on bus. TODO: register as org/weechat/1, or /2, etc. */
    dbus_bus_request_name (ctx->conn, WEECHAT_DBUS_NAME,
                          DBUS_NAME_FLAG_ALLOW_REPLACEMENT | DBUS_NAME_FLAG_DO_NOT_QUEUE,
                          &err);
    if (dbus_error_is_set (&err))
    {
        weechat_printf (NULL,
                        _("%s%s: Error registering as '%s' on DBus: %s"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                        WEECHAT_DBUS_NAME, err.message);
        dbus_error_free (&err);
        goto error;
    }

    ctx->factory = weechat_dbus_object_factory_new (ctx->conn);
    if (!ctx->factory)
    {
        weechat_printf (NULL,
                        _("%s%s: Error creating dbus object factory"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        goto error;
    }

    rc = weechat_dbus_object_factory_make_all_buffers (ctx->factory);
    if (WEECHAT_RC_OK != rc)
    {
        weechat_printf (NULL,
                        _("%s%s: Error adding initial buffers"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        goto error;
    }

    return WEECHAT_RC_OK;

error:
    if (ctx->factory)
    {
        weechat_dbus_object_factory_free (ctx->factory);
    }
    if (ctx->conn)
    {
        dbus_connection_unref (ctx->conn);
        ctx->conn = NULL;
    }
    free (ctx);
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

    if (ctx->main) weechat_dbus_unhook_mainloop (ctx);
    dbus_bus_release_name (ctx->conn, WEECHAT_DBUS_NAME, NULL);
    if (ctx->sigctx) weechat_dbus_unhook_signals (ctx);
    if (ctx->factory) weechat_dbus_object_factory_free (ctx->factory);
    if (ctx->conn) dbus_connection_unref (ctx->conn);
    free (ctx);

    weechat_dbus_plugin = NULL;
    return WEECHAT_RC_OK;
}

/* old dbus-signal */
static int
weechat_dbus_signal_cb_default(void *, const char *, const char *, void *);
static int
weechat_dbus_signal_cb_irc_inout(void *, const char *, const char *, void *);
static int
weechat_dbus_signal_cb_commalist(void *, const char *, const char *, void *);
static int
weechat_dbus_signal_cb_commasplit(void *, const char *, const char *, void *);
static int
weechat_dbus_signal_cb_infolist(void *, const char *, const char *, void *);

struct t_sigmap
{
    const char *weechat_signal;
    const char *path;
    const char *iface;
    const char *name;
    int (*callback)(void *data,
                    const char *signal,
                    const char *type_data,
                    void *signal_data);

};

#define WEECHAT_DBUS_IFACE_SIGNAL_SCRIPT(script) "org.weechat.signal." script

#define SIGMAP_SCRIPT(script) {                                                 \
                                   script "_script_loaded",                     \
                                   WEECHAT_DBUS_OBJECT_SIGNAL,                  \
                                   WEECHAT_DBUS_IFACE_SIGNAL_SCRIPT(script),    \
                                   WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_LOADED,    \
                                   &weechat_dbus_signal_cb_default              \
                               },                                               \
                               {                                                \
                                   script "_script_unloaded",                   \
                                   WEECHAT_DBUS_OBJECT_SIGNAL,                  \
                                   WEECHAT_DBUS_IFACE_SIGNAL_SCRIPT(script),    \
                                   WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_UNLOADED,  \
                                   &weechat_dbus_signal_cb_default              \
                               },                                               \
                               {                                                \
                                   script "_script_installed",                  \
                                   WEECHAT_DBUS_OBJECT_SIGNAL,                  \
                                   WEECHAT_DBUS_IFACE_SIGNAL_SCRIPT(script),    \
                                   WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_INSTALLED, \
                                   &weechat_dbus_signal_cb_commalist            \
                               },                                               \
                               {                                                \
                                   script "_script_removed",                    \
                                   WEECHAT_DBUS_OBJECT_SIGNAL,                  \
                                   WEECHAT_DBUS_IFACE_SIGNAL_SCRIPT(script),    \
                                   WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_REMOVED,   \
                                   &weechat_dbus_signal_cb_commalist            \
                               }

struct t_sigmap sigmap[] =
{
    {
        "*,irc_in2_*",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_IN,
        &weechat_dbus_signal_cb_irc_inout
    },
    {
        "*,irc_out1_*",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_OUT,
        &weechat_dbus_signal_cb_irc_inout
    },
    {
        "irc_ctcp",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_CTCP,
        &weechat_dbus_signal_cb_default
    },
    {
        "irc_dcc",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_DCC,
        &weechat_dbus_signal_cb_default
    },
    {
        "irc_pv",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_PV,
        &weechat_dbus_signal_cb_default
    },
    {
        "irc_server_connecting",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_CONNECTING,
        &weechat_dbus_signal_cb_default
    },
    {
        "irc_server_connected",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_CONNECTED,
        &weechat_dbus_signal_cb_default
    },
    {
        "irc_server_disconnected",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_DISCONNECTED,
        &weechat_dbus_signal_cb_default
    },
    {
        "irc_notify_join",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_JOIN,
        &weechat_dbus_signal_cb_commasplit
    },
    {
        "irc_notify_quit",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_QUIT,
        &weechat_dbus_signal_cb_commasplit
    },
    {
        "irc_notify_away",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_AWAY,
        &weechat_dbus_signal_cb_commasplit
    },
    {
        "irc_notify_still_away",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_STILL_AWAY,
        &weechat_dbus_signal_cb_commasplit
    },
    {
        "irc_notify_back",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_IRC,
        WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_BACK,
        &weechat_dbus_signal_cb_commasplit
    },
    {
        "day_changed",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_CORE,
        WEECHAT_DBUS_MEMBER_SIGNAL_CORE_DAY_CHANGED,
        &weechat_dbus_signal_cb_default
    },
    {
        "plugin_loaded",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_CORE,
        WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PLUGIN_LOADED,
        &weechat_dbus_signal_cb_default
    },
    {
        "plugin_unloaded",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_CORE,
        WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PLUGIN_UNLOADED,
        &weechat_dbus_signal_cb_default
    },
    {
        "quit",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_CORE,
        WEECHAT_DBUS_MEMBER_SIGNAL_CORE_QUIT,
        &weechat_dbus_signal_cb_default
    },
    {
        "upgrade",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_CORE,
        WEECHAT_DBUS_MEMBER_SIGNAL_CORE_UPGRADE,
        &weechat_dbus_signal_cb_default
    },
    {
        "weechat_highlight",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_CORE,
        WEECHAT_DBUS_MEMBER_SIGNAL_CORE_HIGHLIGHT,
        &weechat_dbus_signal_cb_default
    },
    {
        "weechat_pv",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_CORE,
        WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PV,
        &weechat_dbus_signal_cb_default
    },
    {
        "xfer_add",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_XFER,
        WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ADD,
        &weechat_dbus_signal_cb_infolist
    },
    {
        "xfer_send_ready",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_XFER,
        WEECHAT_DBUS_MEMBER_SIGNAL_XFER_SEND_READY,
        &weechat_dbus_signal_cb_infolist
    },
    {
        "xfer_accept_resume",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_XFER,
        WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ACCEPT_RESUME,
        &weechat_dbus_signal_cb_infolist
    },
    {
        "xfer_send_accept_resume",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_XFER,
        WEECHAT_DBUS_MEMBER_SIGNAL_XFER_SEND_ACCEPT_RESUME,
        &weechat_dbus_signal_cb_infolist
    },
    {
        "xfer_start_resume",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_XFER,
        WEECHAT_DBUS_MEMBER_SIGNAL_XFER_START_RESUME,
        &weechat_dbus_signal_cb_infolist
    },
    {
        "xfer_resume_ready",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_XFER,
        WEECHAT_DBUS_MEMBER_SIGNAL_XFER_RESUME_READY,
        &weechat_dbus_signal_cb_infolist
    },
    {
        "xfer_ended",
        WEECHAT_DBUS_OBJECT_SIGNAL,
        WEECHAT_DBUS_IFACE_SIGNAL_XFER,
        WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ENDED,
        &weechat_dbus_signal_cb_infolist
    },
    SIGMAP_SCRIPT("guile"),
    SIGMAP_SCRIPT("lua"),
    SIGMAP_SCRIPT("perl"),
    SIGMAP_SCRIPT("python"),
    SIGMAP_SCRIPT("ruby"),
    SIGMAP_SCRIPT("tcl"),
    { NULL, NULL, NULL, NULL, NULL }
};

struct t_dbus_signal_ctx
{
    struct t_hashtable *ht;
};

static int
weechat_dbus_signal_cb_infolist (void *data,
                                 const char *signal,
                                 const char *type_data,
                                 void *signal_data)
{
    (void) type_data;
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    struct t_sigmap *sigptr = (struct t_sigmap*)weechat_hashtable_get(ctx->sigctx->ht, signal);
    struct t_infolist *infolist = (struct t_infolist*)signal_data;

    if (sigptr)
    {
        DBusMessage *msg = dbus_message_new_signal(sigptr->path, sigptr->iface, sigptr->name);
        if (!msg)
            goto error;

        if (0 > weechat_dbus_infolist_append(msg, infolist))
        {
            dbus_message_unref(msg);
            goto error;
        }

        dbus_connection_send(ctx->conn, msg, NULL);
        dbus_message_unref(msg);
    }

    return WEECHAT_RC_OK;

error:
    weechat_printf (NULL,
                    _("%s%s: out of memory"),
                    weechat_prefix ("error"), DBUS_PLUGIN_NAME);
    return WEECHAT_RC_ERROR;
}

static int
weechat_dbus_signal_cb_irc_inout (void *data,
                                  const char *signal,
                                  const char *type_data,
                                  void *signal_data)
{
    (void) type_data;
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    const char *network, *name, *msgtype;
    char *signal_copy = strdup(signal);
    if (!signal_copy)
        return WEECHAT_RC_OK;

    char *p = strchr (signal_copy, ',');
    if (!p) goto error;
    network = signal_copy;
    *p = '\0';
    p = strchr(++p, '_');
    if (!p) goto error;
    name = ++p;
    p = strrchr(p, '_');
    if (!p) goto error;
    *p = '\0';
    msgtype = ++p;

    int is_in = strncmp(WEECHAT_DBUS_MEMBER_SIGNAL_IRC_IN, name, strlen(name));
    const char *str = (char *)signal_data;
    DBusMessage *msg = dbus_message_new_signal (sigmap[0].path,
                                                sigmap[0].iface,
                                                (0 == is_in) ?
                                                WEECHAT_DBUS_MEMBER_SIGNAL_IRC_IN :
                                                WEECHAT_DBUS_MEMBER_SIGNAL_IRC_OUT);
    if (!msg)
        goto error;

    if (!dbus_message_append_args (msg,
                                   DBUS_TYPE_STRING, &network,
                                   DBUS_TYPE_STRING, &msgtype,
                                   DBUS_TYPE_STRING, &str,
                                   DBUS_TYPE_INVALID))
    {
        dbus_message_unref (msg);
        goto error;
    }

    dbus_connection_send (ctx->conn, msg, NULL);
    dbus_message_unref (msg);
    free (signal_copy);

    return WEECHAT_RC_OK;

error:
    free (signal_copy);
    return WEECHAT_RC_OK;
}

/* Constructs a "as" array of strings reply */
static int
weechat_dbus_signal_cb_commalist (void *data,
                                  const char *signal,
                                  const char *type_data,
                                  void *signal_data)
{
    (void) type_data;
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    struct t_sigmap *sigptr = (struct t_sigmap*)weechat_hashtable_get(ctx->sigctx->ht, signal);
    if (sigptr)
    {
        DBusMessage *msg = dbus_message_new_signal(sigptr->path, sigptr->iface, sigptr->name);
        if (!msg)
            goto error;

        int num_items;
        char **strs = weechat_string_split ((char*)signal_data, ",", 0, 0, &num_items);
        if (!dbus_message_append_args (msg, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &strs, num_items, DBUS_TYPE_INVALID))
        {
            weechat_string_free_split(strs);
            dbus_message_unref(msg);
            goto error;
        }

        dbus_connection_send(ctx->conn, msg, NULL);
        dbus_message_unref(msg);
        weechat_string_free_split(strs);
    }

    return WEECHAT_RC_OK;

error:
    weechat_printf (NULL,
                    _("%s%s: out of memory"),
                    weechat_prefix ("error"), DBUS_PLUGIN_NAME);
    return WEECHAT_RC_ERROR;
}

/* Constructs a "ss" or "sss" or "ssss..." etc reply */
static int
weechat_dbus_signal_cb_commasplit (void *data,
                                  const char *signal,
                                  const char *type_data,
                                  void *signal_data)
{
    (void) type_data;
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    struct t_sigmap *sigptr = (struct t_sigmap*)weechat_hashtable_get(ctx->sigctx->ht, signal);
    if (sigptr)
    {
        DBusMessage *msg = dbus_message_new_signal(sigptr->path, sigptr->iface, sigptr->name);
        if (!msg)
            goto error;

        int num_items;
        char **strs = weechat_string_split ((char*)signal_data, ",", 0, 0, &num_items);
        int i;
        for (i = 0; i < num_items; ++i)
        {
            if (!dbus_message_append_args (msg, DBUS_TYPE_STRING, &(strs[i]), DBUS_TYPE_INVALID))
            {
                weechat_string_free_split(strs);
                dbus_message_unref(msg);
                goto error;
            }
        }

        dbus_connection_send(ctx->conn, msg, NULL);
        dbus_message_unref(msg);
        weechat_string_free_split(strs);
    }

    return WEECHAT_RC_OK;

error:
    weechat_printf (NULL,
                    _("%s%s: out of memory"),
                    weechat_prefix ("error"), DBUS_PLUGIN_NAME);
    return WEECHAT_RC_ERROR;
}

static int
weechat_dbus_signal_cb_default(void *data,
                               const char *signal,
                               const char *type_data,
                               void *signal_data)
{
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    struct t_sigmap *sigptr;

    sigptr = (struct t_sigmap*)weechat_hashtable_get(ctx->sigctx->ht, signal);
    if (sigptr)
    {
        DBusMessage *msg = dbus_message_new_signal(sigptr->path, sigptr->iface,
                                                   sigptr->name);
        if (!msg)
            goto error;

        if (type_data && signal_data)
        {
            if (strcmp(type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
            {
                const char *str = (char*)signal_data;
                if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &str,
                                              DBUS_TYPE_INVALID))
                {
                    dbus_message_unref(msg);
                    goto error;
                }
            }
            if (strcmp(type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
            {
                int64_t fixedint;
                if (sizeof(int) <= sizeof(int64_t))
                {
                    fixedint = (int64_t) *((int*)signal_data);
                }
                else
                {
                    weechat_printf (NULL,
                                    _("%s%s: Error converting int to int64"),
                                    weechat_prefix ("error"), DBUS_PLUGIN_NAME);
                    dbus_message_unref(msg);
                    return WEECHAT_RC_ERROR;
                }

                if (!dbus_message_append_args(msg, DBUS_TYPE_INT64, &fixedint, DBUS_TYPE_INVALID))
                {
                    dbus_message_unref(msg);
                    goto error;
                }

            }
            if (strcmp(type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
            {
                /* We probably shouldn't send pointers over DBus */
            }
        }

        dbus_connection_send(ctx->conn, msg, NULL);
        dbus_message_unref(msg);
    }

    return WEECHAT_RC_OK;

error:
    weechat_printf (NULL,
                    _("%s%s: out of memory"),
                    weechat_prefix ("error"), DBUS_PLUGIN_NAME);
    return WEECHAT_RC_ERROR;
}

int
weechat_dbus_hook_signals(struct t_dbus_ctx *ctx)
{
    struct t_dbus_signal_ctx *sigctx = calloc (1, sizeof(struct t_dbus_signal_ctx));
    if (!sigctx)
        return -1;

    sigctx->ht = weechat_hashtable_new (sizeof(sigmap) / sizeof(struct t_sigmap),
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_POINTER,
                                        NULL,
                                        NULL);
    if (!sigctx->ht)
        goto error;

    ctx->sigctx = sigctx;
    struct t_sigmap *iter = &sigmap[0];

    for (; iter->weechat_signal != NULL; ++iter)
    {
        weechat_hook_signal (iter->weechat_signal, iter->callback, ctx);
        weechat_hashtable_set (ctx->sigctx->ht, iter->weechat_signal, iter);
    }

    (void) ctx;

    return 0;

error:
    if (sigctx->ht)
        weechat_hashtable_free(sigctx->ht);
    if (sigctx)
        free(sigctx);
    return -1;
}

void
weechat_dbus_unhook_signals(struct t_dbus_ctx *ctx)
{
    /* Free everything */
    weechat_hashtable_free(ctx->sigctx->ht);
    free(ctx->sigctx);
    ctx->sigctx = NULL;
}
