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

#if _POSIX_C_SOURCE >= 200112L
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "../weechat-plugin.h"
#include "dbus.h"
#include "dbus-mainloop.h"

static void
weechat_dbus_unhook (void *memory)
{
    struct t_hook *hook = (struct t_hook*)memory;
    if (hook)
        weechat_unhook (hook);
}

static int
weechat_dbus_dispatch(void *data, int remaining_calls)
{
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    (void) remaining_calls;

    DBusDispatchStatus status = dbus_connection_dispatch (ctx->conn);

    if (status == DBUS_DISPATCH_DATA_REMAINS)
        return WEECHAT_RC_OK;

    if (ctx->dispatch)
    {
        weechat_unhook (ctx->dispatch);
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
                            _("%s%s: not enough memory set dispatch"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME);
            /* Fallthrough */
        case DBUS_DISPATCH_COMPLETE:
            if (ctx->dispatch)
            {
                weechat_unhook (ctx->dispatch);
                ctx->dispatch = NULL;
            }
            break;
        case DBUS_DISPATCH_DATA_REMAINS:
            if (!ctx->dispatch)
            {
                ctx->dispatch = weechat_hook_timer (10, 0, 0, weechat_dbus_dispatch, data);
            }
            break;
    };
}

struct t_dbus_watch
{
    DBusWatch *read;
    DBusWatch *write;
    struct t_hook *hook;
    int fd;
    bool hooked_read;
    bool hooked_write;
};


static int
weechat_dbus_watch_cb(void *data, int fd)
{
    struct t_dbus_watch *w = (struct t_dbus_watch*)data;
    bool got_read = false;
    bool got_write = false;
    bool got_error = false;

    if (fd != w->fd)
    {
        weechat_printf (NULL,
                        _("%s%s: fd associated to wrong watch"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        return WEECHAT_RC_OK;
    }

    /* libdbus wants the flags, so we need to do a nonblocking
     * select() */
    {
        fd_set read_fds, write_fds, except_fds;
        struct timeval tv = { 0, 0 };
        FD_ZERO (&read_fds);
        FD_ZERO (&write_fds);
        FD_ZERO (&except_fds);

        if (w->hooked_read)
            FD_SET (fd, &read_fds);
        if (w->hooked_write)
            FD_SET (fd, &write_fds);
        FD_SET (fd, &except_fds);

        int res = select (fd + 1, &read_fds, &write_fds, &except_fds, &tv);
        if (res == 0)
        {
            weechat_printf (NULL,
                            _("%s%s: Unexpected select() behavior"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }
        if (res == -1)
        {
            int err = errno;
            weechat_printf (NULL,
                            _("%s%s: Unexpected select() error: %s"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME,
                            strerror (err));
            return WEECHAT_RC_OK;
        }

        got_read = !!(FD_ISSET(fd, &read_fds));
        got_write = !!(FD_ISSET(fd, &write_fds));
        got_error = !!(FD_ISSET(fd, &except_fds));
    }

    if ((got_read || got_error) && w->read)
    {
        dbus_bool_t ret = dbus_watch_handle (w->read, DBUS_WATCH_READABLE);
        if (!ret)
        {
            weechat_printf (NULL,
                            _("%s%s: not enough memory"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }
    }

    if ((got_write || got_error) && w->write)
    {
        dbus_bool_t ret = dbus_watch_handle (w->write, DBUS_WATCH_WRITABLE);
        if (!ret)
        {
            weechat_printf (NULL,
                            _("%s%s: not enough memory"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }
    }

    return WEECHAT_RC_OK;
}

void
weechat_dbus_watch_toggled(DBusWatch *watch, void *data)
{
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    struct t_dbus_watch *w = (struct t_dbus_watch*)dbus_watch_get_data(watch);
    unsigned int flags = dbus_watch_get_flags(watch);
    bool is_read = !!(flags & DBUS_WATCH_READABLE);
    bool is_write = !!(flags & DBUS_WATCH_WRITABLE);

    if (dbus_watch_get_enabled(watch))
    {
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
        int fd = dbus_watch_get_socket(watch);
#else
        int fd = dbus_watch_get_unix_fd(watch);
#endif
        if (fd != w->fd)
        {
            weechat_printf (NULL,
                            _("%s%s: Watch created for fd %d but now wants to read %d"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME, w->fd, fd);
            return;
        }

        struct t_dbus_watch *htw = (struct t_dbus_watch*)weechat_hashtable_get(ctx->hook_table, &fd);
        if (htw != w)
        {
            weechat_printf (NULL,
                            _("%s%s: Several t_dbus_watch are on fd %d"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME, fd);
        }

        if (is_read && watch != w->read && w->read != NULL)
        {
            weechat_printf (NULL,
                            _("%s%s: Another watch is already reading %d"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME, fd);
            return;
        }
        if (is_write && watch != w->write && w->write != NULL)
        {
            weechat_printf (NULL,
                            _("%s%s: Another watch is already writing %d"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME, fd);
            return;
        }
        if (is_write && w->write == NULL)
        {
            w->write = watch;
        }
        if (is_read && w->read == NULL)
        {
            w->read = watch;
        }
        if (w->hook)
        {
            weechat_unhook (w->hook);
        }
        w->hooked_read = w->hooked_read || is_read;
        w->hooked_write = w->hooked_write || is_write;

        w->hook = weechat_hook_fd (fd,
                                   w->hooked_read?1:0,
                                   w->hooked_write?1:0,
                                   1, &weechat_dbus_watch_cb, w);
        if (!w->hook)
        {
            weechat_printf (NULL,
                            _("%s%s: couldn't hook fd %d"),
                            weechat_prefix ("error"), DBUS_PLUGIN_NAME, fd);
            return;
        }
    }
    else
    {
        if (w->read == watch)
        {
            w->hooked_read = false;
            if (!is_read)
                w->read = NULL;
        }
        if (w->write == watch)
        {
            w->hooked_write = false;
            if (!is_write)
                w->write = NULL;
        }
        if (w->hook)
        {
            weechat_unhook (w->hook);
            w->hook = NULL;
            if (w->hooked_read || w->hooked_write)
            {
                w->hook = weechat_hook_fd (w->fd,
                                           w->hooked_read?1:0,
                                           w->hooked_write?1:0,
                                           1, &weechat_dbus_watch_cb, w);
                if (!w->hook)
                {
                    weechat_printf (NULL,
                                    _("%s%s: couldn't hook fd %d"),
                                    weechat_prefix ("error"), DBUS_PLUGIN_NAME, w->fd);
                }
            }
        }
    }
}

dbus_bool_t
weechat_dbus_add_watch(DBusWatch *watch, void *data)
{
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
#if defined _WIN32 || defined __WIN32__ || defined __CYGWIN__
    int fd = dbus_watch_get_socket(watch);
#else
    int fd = dbus_watch_get_unix_fd(watch);
#endif
    unsigned int flags = dbus_watch_get_flags(watch);
    bool is_read = !!(flags & DBUS_WATCH_READABLE);
    bool is_write = !!(flags & DBUS_WATCH_WRITABLE);

    struct t_dbus_watch *w = (struct t_dbus_watch*)weechat_hashtable_get (ctx->hook_table, &fd);
    if (w)
    {
        if (is_read)
        {
            if (w->read)
            {
                weechat_printf (NULL,
                                _("%s%s: Unexpected read add_watch when already reading"),
                                weechat_prefix ("error"), DBUS_PLUGIN_NAME);
                return FALSE;
            }
            w->read = watch;
        }
        else if (is_write)
        {
            if (w->write)
            {
                weechat_printf (NULL,
                                _("%s%s: Unexpected watch add_watch when already writing"),
                                weechat_prefix ("error"), DBUS_PLUGIN_NAME);
                return FALSE;
            }
            w->write = watch;
        }
    }
    else
    {
        w = malloc (sizeof(struct t_dbus_watch));
        if (!w)
            return FALSE;
        w->read = is_read?watch:NULL;
        w->write = is_write?watch:NULL;
        w->hook = NULL;
        w->fd = fd;
        w->hooked_read = false;
        w->hooked_write = false;
        weechat_hashtable_set (ctx->hook_table, &fd, w);
    }

    dbus_watch_set_data (watch, w, NULL);

    if (dbus_watch_get_enabled (watch))
        weechat_dbus_watch_toggled (watch, data);
    
    return TRUE;
}

void
weechat_dbus_remove_watch(DBusWatch *watch, void *data)
{
    struct t_dbus_ctx *ctx = (struct t_dbus_ctx*)data;
    struct t_dbus_watch *w = (struct t_dbus_watch*)dbus_watch_get_data (watch);
    
    if (w->read == watch)
    {
        if (w->hooked_read)
        {
            weechat_unhook (w->hook);
            w->hooked_read = false;
            if (w->hooked_write)
            {
                w->hook = weechat_hook_fd (w->fd, 0, 1, 1, &weechat_dbus_watch_cb, w);
            }
        }
        w->read = NULL;
    }
    if (w->write == watch)
    {
        if (w->hooked_write)
        {
            weechat_unhook (w->hook);
            w->hooked_write = false;
            if (w->hooked_read)
            {
                w->hook = weechat_hook_fd (w->fd, 1, 0, 1, &weechat_dbus_watch_cb, w);
            }
        }
        w->write = NULL;
    }

    dbus_watch_set_data (watch, NULL, NULL);

    if (!w->read && !w->write && !w->hook)
    {
        weechat_hashtable_remove (ctx->hook_table, &w->fd);
        free (w);
    }
}

static int
weechat_dbus_timeout_cb(void *data, int remaining_calls)
{
    DBusTimeout *timeout = (DBusTimeout*)data;
    (void) remaining_calls;

    if (!dbus_timeout_handle (timeout))
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory timeout cb"),
                        weechat_prefix ("error"), DBUS_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    return WEECHAT_RC_OK;
}

dbus_bool_t
weechat_dbus_add_timeout(DBusTimeout *timeout, void *data)
{
    dbus_timeout_set_data (timeout, NULL, NULL);

    if (dbus_timeout_get_enabled (timeout))
        weechat_dbus_timeout_toggled (timeout, data);

    return TRUE;
}

void
weechat_dbus_remove_timeout(DBusTimeout *timeout, void *data)
{
    (void) data;

    dbus_timeout_set_data (timeout, NULL, NULL);
}

void
weechat_dbus_timeout_toggled(DBusTimeout *timeout, void *data)
{
    (void) data;
    struct t_hook *hook = (struct t_hook*) dbus_timeout_get_data (timeout);
    
    if (dbus_timeout_get_enabled (timeout))
    {
        long interval = (long)dbus_timeout_get_interval (timeout);
        hook = weechat_hook_timer (interval, 0, 0, &weechat_dbus_timeout_cb,
                                   timeout);
        dbus_timeout_set_data (timeout, hook, weechat_dbus_unhook);
    }
    else
    {
        dbus_timeout_set_data (timeout, NULL, NULL);
    }
}
