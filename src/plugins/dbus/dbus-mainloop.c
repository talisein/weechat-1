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
#include <poll.h>
#include <errno.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-mainloop.h"

static void
weechat_dbus_unhook (void *memory)
{
    struct t_hook *hook = (struct t_hook*)memory;
    if (hook)
        weechat_unhook(hook);
}

static int
weechat_dbus_dispatch(void *data, int remaining_calls)
{
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    (void) remaining_calls;

    DBusDispatchStatus status = dbus_connection_dispatch(ctx->conn);

    if (status == DBUS_DISPATCH_DATA_REMAINS)
        return WEECHAT_RC_OK;

    if (ctx->dispatch)
    {
        weechat_unhook(ctx->dispatch);
        ctx->dispatch = NULL;
    }

    return WEECHAT_RC_OK;
}

void
weechat_dbus_set_dispatch(DBusConnection *connection, DBusDispatchStatus new_status, void *data)
{
    (void) connection;
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;

    switch (new_status)
    {
        case DBUS_DISPATCH_NEED_MEMORY:
            weechat_printf (NULL,
                            _("%s%s: not enough memory"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME);
            /* Fallthrough */
        case DBUS_DISPATCH_COMPLETE:
            if (ctx->dispatch)
            {
                weechat_unhook(ctx->dispatch);
                ctx->dispatch = NULL;
            }
            break;
        case DBUS_DISPATCH_DATA_REMAINS:
            if (!ctx->dispatch)
            {
                ctx->dispatch = weechat_hook_timer(10, 0, 0, weechat_dbus_dispatch, data);
            }
            break;
    };
}

static int
weechat_dbus_watch_cb(void *data, int fd)
{
    DBusWatch *watch = (DBusWatch*)data;
    unsigned int watch_flags = dbus_watch_get_flags(watch);

    /* libdbus wants the flags, so we need to do a nonblocking
     * poll() */
    {
        short events = 0;
        events |= watch_flags & DBUS_WATCH_READABLE ? POLLIN:0;
        events |= watch_flags & DBUS_WATCH_READABLE ? POLLPRI:0;
        events |= watch_flags & DBUS_WATCH_WRITABLE ? POLLOUT:0;
        events |= POLLERR | POLLHUP;
    
        struct pollfd pfd = {fd, events, 0};
        int res = poll(&pfd, 1, 0);
        if (res == 0)
        {
            weechat_printf (NULL,
                            _("%s%s: Unexpected behavior in dbus plugin"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME);
            return WEECHAT_RC_ERROR;
        }
        if (res == -1)
        {
            int err = errno;
            weechat_printf (NULL,
                            _("%s%s: Unexpected poll error in dbus plugin: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            strerror(err));
            return WEECHAT_RC_ERROR;
        }
        watch_flags = 0;
        watch_flags |= pfd.revents & POLLIN ? DBUS_WATCH_READABLE:0;
        watch_flags |= pfd.revents & POLLPRI ? DBUS_WATCH_READABLE:0;
        watch_flags |= pfd.revents & POLLOUT ? DBUS_WATCH_WRITABLE:0;
        watch_flags |= pfd.revents & POLLERR ? DBUS_WATCH_ERROR:0;
        watch_flags |= pfd.revents & POLLHUP ? DBUS_WATCH_ERROR:0;
    }

    if (!dbus_watch_handle (watch, watch_flags))
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}

void
weechat_dbus_watch_toggled(DBusWatch *watch, void *data)
{
    (void) data;

    if (dbus_watch_get_enabled(watch))
    {
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
        int fd = dbus_watch_get_socket(watch);
#else
        int fd = dbus_watch_get_unix_fd(watch);
#endif
        unsigned int flags = dbus_watch_get_flags(watch);

        struct t_hook *hook = weechat_hook_fd (fd,
                                               flags & DBUS_WATCH_READABLE?1:0,
                                               flags & DBUS_WATCH_WRITABLE?1:0,
                                               1, &weechat_dbus_watch_cb, watch);
        dbus_watch_set_data (watch, hook, weechat_dbus_unhook);
    }
    else
    {
        dbus_watch_set_data(watch, NULL, NULL);
    }
}

dbus_bool_t
weechat_dbus_add_watch(DBusWatch *watch, void *data)
{
    dbus_watch_set_data(watch, NULL, NULL);

    if (dbus_watch_get_enabled(watch))
        weechat_dbus_watch_toggled(watch, data);

    return TRUE;
}

void
weechat_dbus_remove_watch(DBusWatch *watch, void *data)
{
    (void) data;

    dbus_watch_set_data(watch, NULL, NULL);
}

static int
weechat_dbus_timeout_cb(void *data, int remaining_calls)
{
    DBusTimeout *timeout = (DBusTimeout*)data;
    (void) remaining_calls;

    if (!dbus_timeout_handle(timeout))
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }
    
    return WEECHAT_RC_OK;
}

dbus_bool_t
weechat_dbus_add_timeout(DBusTimeout *timeout, void *data)
{
    dbus_timeout_set_data(timeout, NULL, NULL);

    if (dbus_timeout_get_enabled (timeout))
        weechat_dbus_timeout_toggled(timeout, data);

    return TRUE;
}

void
weechat_dbus_remove_timeout(DBusTimeout *timeout, void *data)
{
    (void) data;
    
    dbus_timeout_set_data(timeout, NULL, NULL);
}

void
weechat_dbus_timeout_toggled(DBusTimeout *timeout, void *data)
{
    (void) data;
    struct t_hook *hook = (struct t_hook*)dbus_timeout_get_data(timeout);
    
    if (dbus_timeout_get_enabled (timeout))
    {
        long interval = (long)dbus_timeout_get_interval (timeout);
        hook = weechat_hook_timer (interval, 0, 0, &weechat_dbus_timeout_cb,
                                   timeout);
        dbus_timeout_set_data(timeout, hook, weechat_dbus_unhook);
    }
    else
    {
        dbus_timeout_set_data(timeout, NULL, NULL);
    }
}
