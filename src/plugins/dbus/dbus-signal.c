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
#include "dbus-signal.h"
#include "dbus-infolist.h"

static int
weechat_dbus_signal_cb_default(void *, const char *, const char *, void *);
static int
weechat_dbus_signal_cb_irc_inout(void *, const char *, const char *, void *);
static int
weechat_dbus_signal_cb_commalist(void *, const char *, const char *, void *);
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

const char WEECHAT_DBUS_SIGNAL_OBJECT[] = "/org/weechat/signal";

#define SIGMAP_SCRIPT(scrname) { scrname "_script_loaded",      \
            WEECHAT_DBUS_SIGNAL_OBJECT,                         \
            "org.weechat.signal." scrname,                      \
            "script_loaded",                                    \
            &weechat_dbus_signal_cb_default                     \
            },                                                  \
        { scrname "_script_unloaded",                           \
                WEECHAT_DBUS_SIGNAL_OBJECT,                     \
                "org.weechat.signal." scrname,                  \
                "script_unloaded",                              \
                &weechat_dbus_signal_cb_default                 \
                },                                              \
        { scrname "_script_installed",                          \
                WEECHAT_DBUS_SIGNAL_OBJECT,                     \
                "org.weechat.signal." scrname,                  \
                "script_isntalled",                             \
                &weechat_dbus_signal_cb_commalist               \
                },                                              \
        { scrname "_script_removed",                            \
                WEECHAT_DBUS_SIGNAL_OBJECT,                     \
                "org.weechat.signal." scrname,                  \
                "script_removed",                               \
                &weechat_dbus_signal_cb_commalist               \
                }

struct t_sigmap sigmap[] =
{
    { "*,irc_in2_*",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "in2",
      &weechat_dbus_signal_cb_irc_inout
    },
    { "*,irc_out1_*",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "out1",
      &weechat_dbus_signal_cb_irc_inout
    },
    { "irc_ctcp",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "ctcp",
      &weechat_dbus_signal_cb_default
    },
    { "irc_dcc",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "dcc",
      &weechat_dbus_signal_cb_default
    },
    { "irc_pv",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "pv",
      &weechat_dbus_signal_cb_default
    },
    { "irc_server_connecting",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "server_connecting",
      &weechat_dbus_signal_cb_default
    },
    { "irc_server_connected",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "server_connected",
      &weechat_dbus_signal_cb_default
    },
    { "irc_server_disconnected",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "server_disconnected",
      &weechat_dbus_signal_cb_default
    },
    { "irc_notify_join",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "notify_join",
      &weechat_dbus_signal_cb_commalist
    },
    { "irc_notify_quit",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "notify_quit",
      &weechat_dbus_signal_cb_commalist
    },
    { "irc_notify_away",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "notify_away",
      &weechat_dbus_signal_cb_commalist
    },
    { "irc_notify_still_away",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "notify_still_away",
      &weechat_dbus_signal_cb_commalist
    },
    { "irc_notify_back",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.irc",
      "notify_back",
      &weechat_dbus_signal_cb_commalist
    },
    { "day_changed",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.core",
      "day_changed",
      &weechat_dbus_signal_cb_default
    },
    { "plugin_loaded",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.core",
      "plugin_loaded",
      &weechat_dbus_signal_cb_default
    },
    { "plugin_unloaded",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.core",
      "plugin_unloaded",
      &weechat_dbus_signal_cb_default
    },
    { "quit",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.core",
      "quit",
      &weechat_dbus_signal_cb_default
    },
    { "upgrade",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.core",
      "upgrade",
      &weechat_dbus_signal_cb_default
    },
    { "weechat_highlight",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.core",
      "highlight",
      &weechat_dbus_signal_cb_default
    },
    { "weechat_pv",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.core",
      "pv",
      &weechat_dbus_signal_cb_default
    },
    { "xfer_add",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.xfer",
      "add",
      &weechat_dbus_signal_cb_infolist
    },
    { "xfer_send_ready",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.xfer",
      "send_ready",
      &weechat_dbus_signal_cb_infolist
    },
    { "xfer_accept_resume",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.xfer",
      "accept_resume",
      &weechat_dbus_signal_cb_infolist
    },
    { "xfer_send_accept_resume",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.xfer",
      "send_accept_resume",
      &weechat_dbus_signal_cb_infolist
    },
    { "xfer_start_resume",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.xfer",
      "start_resume",
      &weechat_dbus_signal_cb_infolist
    },
    { "xfer_resume_ready",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.xfer",
      "resume_ready",
      &weechat_dbus_signal_cb_infolist
    },
    { "xfer_ended",
      WEECHAT_DBUS_SIGNAL_OBJECT,
      "org.weechat.signal.xfer",
      "ended",
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

    const char *str = (char *)signal_data;
    DBusMessage *msg = dbus_message_new_signal (sigmap[0].path,
                                                sigmap[0].iface,
                                                name);
    if (!msg)
        goto error;

    if (!dbus_message_append_args (msg, DBUS_TYPE_STRING, &network,
                                   DBUS_TYPE_STRING, &msgtype, DBUS_TYPE_STRING,
                                   &str, DBUS_TYPE_INVALID))
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

