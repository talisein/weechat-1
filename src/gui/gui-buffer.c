/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* gui-buffer.c: buffer functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-completion.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-input.h"
#include "gui-keyboard.h"
#include "gui-main.h"
#include "gui-nicklist.h"
#include "gui-status.h"
#include "gui-window.h"


struct t_gui_buffer *gui_buffers = NULL;           /* first buffer          */
struct t_gui_buffer *last_gui_buffer = NULL;       /* last buffer           */
struct t_gui_buffer *gui_previous_buffer = NULL;   /* previous buffer       */

char *gui_buffer_notify_string[GUI_BUFFER_NUM_NOTIFY] =
{ "none", "highlight", "message", "all" };


/*
 * gui_buffer_new: create a new buffer in current window
 */

struct t_gui_buffer *
gui_buffer_new (struct t_weechat_plugin *plugin,
                const char *name,
                int (*input_callback)(void *data,
                                      struct t_gui_buffer *buffer,
                                      const char *input_data),
                void *input_callback_data,
                int (*close_callback)(void *data,
                                      struct t_gui_buffer *buffer),
                void *close_callback_data)
{
    struct t_gui_buffer *new_buffer;
    struct t_gui_completion *new_completion;
    
    if (!name)
        return NULL;
    
    if (gui_buffer_search_by_name ((plugin) ? plugin->name : "core", name))
    {
        gui_chat_printf (NULL,
                         _("%sError: a buffer with same name (%s) already "
                           "exists"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        return NULL;
    }
    
    /* create new buffer */
    new_buffer = malloc (sizeof (*new_buffer));
    if (new_buffer)
    {
        /* init buffer */
        new_buffer->plugin = plugin;
        new_buffer->plugin_name_for_upgrade = NULL;
        new_buffer->number = (last_gui_buffer) ? last_gui_buffer->number + 1 : 1;
        new_buffer->name = strdup (name);
        new_buffer->type = GUI_BUFFER_TYPE_FORMATED;
        new_buffer->notify = CONFIG_INTEGER(config_look_buffer_notify_default);
        new_buffer->num_displayed = 0;
        
        /* close callback */
        new_buffer->close_callback = close_callback;
        new_buffer->close_callback_data = close_callback_data;
        
        /* title */
        new_buffer->title = NULL;
        new_buffer->title_refresh_needed = 1;
        
        /* chat lines (formated) */
        new_buffer->lines = NULL;
        new_buffer->last_line = NULL;
        new_buffer->last_read_line = NULL;
        new_buffer->lines_count = 0;
        new_buffer->lines_hidden = 0;
        new_buffer->prefix_max_length = 0;
        new_buffer->chat_refresh_needed = 2;
        
        /* nicklist */
        new_buffer->nicklist = 0;
        new_buffer->nicklist_case_sensitive = 0;
        new_buffer->nicklist_root = NULL;
        new_buffer->nicklist_max_length = 0;
        new_buffer->nicklist_display_groups = 1;
        new_buffer->nicklist_visible_count = 0;
        new_buffer->nicklist_refresh_needed = 1;
        gui_nicklist_add_group (new_buffer, NULL, "root", NULL, 0);
        
        /* input */
        new_buffer->input = 1;
        new_buffer->input_callback = input_callback;
        new_buffer->input_callback_data = input_callback_data;
        new_buffer->input_nick = NULL;
        new_buffer->input_buffer_alloc = GUI_BUFFER_INPUT_BLOCK_SIZE;
        new_buffer->input_buffer = malloc (GUI_BUFFER_INPUT_BLOCK_SIZE);
        new_buffer->input_buffer_color_mask = malloc (GUI_BUFFER_INPUT_BLOCK_SIZE);
        new_buffer->input_buffer[0] = '\0';
        new_buffer->input_buffer_color_mask[0] = '\0';
        new_buffer->input_buffer_size = 0;
        new_buffer->input_buffer_length = 0;
        new_buffer->input_buffer_pos = 0;
        new_buffer->input_buffer_1st_display = 0;
        new_buffer->input_refresh_needed = 1;
        
        /* init completion */
        new_completion = malloc (sizeof (*new_completion));
        if (new_completion)
        {
            new_buffer->completion = new_completion;
            gui_completion_init (new_completion, new_buffer);
        }
        
        /* init history */
        new_buffer->history = NULL;
        new_buffer->last_history = NULL;
        new_buffer->ptr_history = NULL;
        new_buffer->num_history = 0;
        
        /* text search */
        new_buffer->text_search = GUI_TEXT_SEARCH_DISABLED;
        new_buffer->text_search_exact = 0;
        new_buffer->text_search_found = 0;
        new_buffer->text_search_input = NULL;
        
        /* highlight */
        new_buffer->highlight_words = NULL;
        new_buffer->highlight_tags = NULL;
        new_buffer->highlight_tags_count = 0;
        new_buffer->highlight_tags_array = NULL;
        
        /* keys */
        new_buffer->keys = NULL;
        new_buffer->last_key = NULL;
        
        /* add buffer to buffers list */
        new_buffer->prev_buffer = last_gui_buffer;
        if (gui_buffers)
            last_gui_buffer->next_buffer = new_buffer;
        else
            gui_buffers = new_buffer;
        last_gui_buffer = new_buffer;
        new_buffer->next_buffer = NULL;
        
        /* first buffer creation ? */
        if (!gui_current_window->buffer)
        {
            gui_current_window->buffer = new_buffer;
            gui_current_window->first_line_displayed = 1;
            gui_current_window->start_line = NULL;
            gui_current_window->start_line_pos = 0;
            gui_window_calculate_pos_size (gui_current_window, 1);
            gui_window_switch_to_buffer (gui_current_window, new_buffer);
            gui_window_redraw_buffer (new_buffer);
        }
        
        hook_signal_send ("buffer_open",
                          WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);
    }
    else
        return NULL;
    
    return new_buffer;
}

/*
 * gui_buffer_valid: check if a buffer pointer exists
 *                   return 1 if buffer exists
 *                          0 if buffer is not found
 */

int
gui_buffer_valid (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* NULL buffer is valid (it's for printing on first buffer) */
    if (!buffer)
        return 1;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer == buffer)
            return 1;
    }
    
    /* buffer not found */
    return 0;
}

/*
 * gui_buffer_set_plugin_for_upgrade: set plugin pointer for buffers with a
 *                                    given name (used after /upgrade)
 */

void
gui_buffer_set_plugin_for_upgrade (char *name, struct t_weechat_plugin *plugin)
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->plugin_name_for_upgrade
            && (strcmp (ptr_buffer->plugin_name_for_upgrade, name) == 0))
        {
            free (ptr_buffer->plugin_name_for_upgrade);
            ptr_buffer->plugin_name_for_upgrade = NULL;
            
            ptr_buffer->plugin = plugin;
        }
    }
}

/*
 * gui_buffer_get_integer: get a buffer property as integer
 */

int
gui_buffer_get_integer (struct t_gui_buffer *buffer, const char *property)
{
    if (buffer && property)
    {
        if (string_strcasecmp (property, "number") == 0)
            return buffer->number;
        else if (string_strcasecmp (property, "notify") == 0)
            return buffer->notify;
        else if (string_strcasecmp (property, "lines_hidden") == 0)
            return buffer->lines_hidden;
    }
    
    return 0;
}

/*
 * gui_buffer_get_string: get a buffer property as string
 */

char *
gui_buffer_get_string (struct t_gui_buffer *buffer, const char *property)
{
    if (buffer && property)
    {
        if (string_strcasecmp (property, "plugin") == 0)
            return (buffer->plugin) ? buffer->plugin->name : NULL;
        else if (string_strcasecmp (property, "name") == 0)
            return buffer->name;
        else if (string_strcasecmp (property, "title") == 0)
            return buffer->title;
        else if (string_strcasecmp (property, "nick") == 0)
            return buffer->input_nick;
    }
    
    return NULL;
}

/*
 * gui_buffer_get_pointer: get a buffer property as pointer
 */

void *
gui_buffer_get_pointer (struct t_gui_buffer *buffer, const char *property)
{
    if (buffer && property)
    {
        if (string_strcasecmp (property, "plugin") == 0)
            return buffer->plugin;
    }
    
    return NULL;
}

/*
 * gui_buffer_ask_title_refresh: set "title_refresh_needed" flag
 */

void
gui_buffer_ask_title_refresh (struct t_gui_buffer *buffer, int refresh)
{
    if (refresh > buffer->title_refresh_needed)
        buffer->title_refresh_needed = refresh;
}

/*
 * gui_buffer_ask_chat_refresh: set "chat_refresh_needed" flag
 */

void
gui_buffer_ask_chat_refresh (struct t_gui_buffer *buffer, int refresh)
{
    if (refresh > buffer->chat_refresh_needed)
        buffer->chat_refresh_needed = refresh;
}

/*
 * gui_buffer_ask_nicklist_refresh: set "nicklist_refresh_needed" flag
 */

void
gui_buffer_ask_nicklist_refresh (struct t_gui_buffer *buffer, int refresh)
{
    if (refresh > buffer->nicklist_refresh_needed)
        buffer->nicklist_refresh_needed = refresh;
}

/*
 * gui_buffer_ask_input_refresh: set "input_refresh_needed" flag
 */

void
gui_buffer_ask_input_refresh (struct t_gui_buffer *buffer, int refresh)
{
    if (refresh > buffer->input_refresh_needed)
        buffer->input_refresh_needed = refresh;
}

/*
 * gui_buffer_set_name: set name for a buffer
 */

void
gui_buffer_set_name (struct t_gui_buffer *buffer, const char *name)
{
    if (name && name[0])
    {
        if (buffer->name)
            free (buffer->name);
        buffer->name = strdup (name);
        
        gui_status_refresh_needed = 1;
        
        hook_signal_send ("buffer_renamed",
                          WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * gui_buffer_set_type: set buffer type
 */

void
gui_buffer_set_type (struct t_gui_buffer *buffer, enum t_gui_buffer_type type)
{
    if (buffer->type == type)
        return;
    
    gui_chat_line_free_all (buffer);
    
    buffer->type = type;
    gui_buffer_ask_chat_refresh (buffer, 2);
}

/*
 * gui_buffer_set_title: set title for a buffer
 */

void
gui_buffer_set_title (struct t_gui_buffer *buffer, const char *new_title)
{
    if (buffer->title)
        free (buffer->title);
    buffer->title = (new_title && new_title[0]) ? strdup (new_title) : NULL;
    gui_buffer_ask_title_refresh (buffer, 1);
    hook_signal_send ("buffer_title_changed", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * gui_buffer_set_nicklist: set nicklist for a buffer
 */

void
gui_buffer_set_nicklist (struct t_gui_buffer *buffer, int nicklist)
{
    buffer->nicklist = (nicklist) ? 1 : 0;
    gui_window_refresh_windows ();
}

/*
 * gui_buffer_set_nicklist_case_sensitive: set case_sensitive flag for a buffer
 */

void
gui_buffer_set_nicklist_case_sensitive (struct t_gui_buffer *buffer,
                                        int case_sensitive)
{
    buffer->nicklist_case_sensitive = (case_sensitive) ? 1 : 0;
}

/*
 * gui_buffer_set_nicklist_display_groups: set display_groups flag for a buffer
 */

void
gui_buffer_set_nicklist_display_groups (struct t_gui_buffer *buffer,
                                        int display_groups)
{
    buffer->nicklist_display_groups = (display_groups) ? 1 : 0;
    buffer->nicklist_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    gui_buffer_ask_nicklist_refresh (buffer, 1);
}

/*
 * gui_buffer_set_nick: set nick for a buffer
 */

void
gui_buffer_set_nick (struct t_gui_buffer *buffer, const char *new_nick)
{
    if (buffer->input_nick)
        free (buffer->input_nick);
    buffer->input_nick = (new_nick && new_nick[0]) ? strdup (new_nick) : NULL;
    gui_buffer_ask_input_refresh (buffer, 1);
}

/*
 * gui_buffer_set_highlight_words: set highlight words for a buffer
 */

void
gui_buffer_set_highlight_words (struct t_gui_buffer *buffer,
                                const char *new_highlight_words)
{
    if (buffer->highlight_words)
        free (buffer->highlight_words);
    buffer->highlight_words = (new_highlight_words && new_highlight_words[0]) ?
        strdup (new_highlight_words) : NULL;
}

/*
 * gui_buffer_set_highlight_tags: set highlight tags for a buffer
 */

void
gui_buffer_set_highlight_tags (struct t_gui_buffer *buffer,
                               const char *new_highlight_tags)
{
    if (buffer->highlight_tags)
        free (buffer->highlight_tags);
    if (buffer->highlight_tags_array)
        string_free_exploded (buffer->highlight_tags_array);
    
    if (new_highlight_tags)
    {
        buffer->highlight_tags = strdup (new_highlight_tags);
        if (buffer->highlight_tags)
        {
            buffer->highlight_tags_array = string_explode (new_highlight_tags,
                                                           ",", 0, 0,
                                                           &buffer->highlight_tags_count);
        }
    }
    else
    {
        buffer->highlight_tags = NULL;
        buffer->highlight_tags_count = 0;
        buffer->highlight_tags_array = NULL;
    }
}

/*
 * gui_buffer_set: set a buffer property
 */

void
gui_buffer_set (struct t_gui_buffer *buffer, const char *property,
                void *value)
{
    const char *value_str;
    long number;
    char *error;
    
    if (!property || !value)
        return;
    
    value_str = (const char *)value;
    
    /* properties that does NOT need a buffer */
    if (string_strcasecmp (property, "hotlist") == 0)
    {
        if (strcmp (value_str, "-") == 0)
            gui_add_hotlist = 0;
        else if (strcmp (value_str, "+") == 0)
            gui_add_hotlist = 1;
        else
        {
            error = NULL;
            number = strtol (value_str, &error, 10);
            if (error && !error[0])
                gui_hotlist_add (buffer, number, NULL, 1);
        }
    }
    
    if (!buffer)
        return;
    
    /* properties that need a buffer */
    if (string_strcasecmp (property, "close_callback") == 0)
    {
        buffer->close_callback = value;
    }
    else if (string_strcasecmp (property, "close_callback_data") == 0)
    {
        buffer->close_callback_data = value;
    }
    else if (string_strcasecmp (property, "input_callback") == 0)
    {
        buffer->input_callback = value;
    }
    else if (string_strcasecmp (property, "input_callback_data") == 0)
    {
        buffer->input_callback_data = value;
    }
    else if (string_strcasecmp (property, "display") == 0)
    {
        gui_window_switch_to_buffer (gui_current_window, buffer);
        gui_window_redraw_buffer (buffer);
    }
    else if (string_strcasecmp (property, "name") == 0)
    {
        gui_buffer_set_name (buffer, value_str);
    }
    else if (string_strcasecmp (property, "type") == 0)
    {
        if (string_strcasecmp (value_str, "formated") == 0)
            gui_buffer_set_type (buffer, GUI_BUFFER_TYPE_FORMATED);
        else if (string_strcasecmp (value_str, "free") == 0)
            gui_buffer_set_type (buffer, GUI_BUFFER_TYPE_FREE);
    }
    else if (string_strcasecmp (property, "notify") == 0)
    {
        error = NULL;
        number = strtol (value_str, &error, 10);
        if (error && !error[0]
            && (number < GUI_BUFFER_NUM_NOTIFY))
        {
            if (number < 0)
                buffer->notify = CONFIG_INTEGER(config_look_buffer_notify_default);
            else
                buffer->notify = number;
        }
    }
    else if (string_strcasecmp (property, "title") == 0)
    {
        gui_buffer_set_title (buffer, value_str);
    }
    else if (string_strcasecmp (property, "nicklist") == 0)
    {
        error = NULL;
        number = strtol (value_str, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist (buffer, number);
    }
    else if (string_strcasecmp (property, "nicklist_case_sensitive") == 0)
    {
        error = NULL;
        number = strtol (value_str, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist_case_sensitive (buffer, number);
    }
    else if (string_strcasecmp (property, "nicklist_display_groups") == 0)
    {
        error = NULL;
        number = strtol (value_str, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist_display_groups (buffer, number);
    }
    else if (string_strcasecmp (property, "nick") == 0)
    {
        gui_buffer_set_nick (buffer, value_str);
    }
    else if (string_strcasecmp (property, "highlight_words") == 0)
    {
        gui_buffer_set_highlight_words (buffer, value_str);
    }
    else if (string_strcasecmp (property, "highlight_tags") == 0)
    {
        gui_buffer_set_highlight_tags (buffer, value_str);
    }
    else if (string_strncasecmp (property, "key_bind_", 9) == 0)
    {
        gui_keyboard_bind (buffer, property + 9, value_str);
    }
    else if (string_strncasecmp (property, "key_unbind_", 11) == 0)
    {
        if (strcmp (property + 11, "*") == 0)
            gui_keyboard_free_all (&buffer->keys, &buffer->last_key);
        else
            gui_keyboard_unbind (buffer, property + 11);
    }
    else if (string_strcasecmp (property, "input") == 0)
    {
        gui_input_delete_line (buffer);
        gui_input_insert_string (buffer, value_str, 0);
        gui_input_text_changed_signal ();
        gui_buffer_ask_input_refresh (buffer, 1);
    }
}

/*
 * gui_buffer_search_main: get main buffer (weechat one, created at startup)
 *                         return first buffer if not found
 */

struct t_gui_buffer *
gui_buffer_search_main ()
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (!ptr_buffer->plugin)
            return ptr_buffer;
    }
    
    /* buffer not found, return first buffer by default */
    return gui_buffers;
}

/*
 * gui_buffer_search_by_name: search a buffer by name
 */

struct t_gui_buffer *
gui_buffer_search_by_name (const char *plugin, const char *name)
{
    struct t_gui_buffer *ptr_buffer;
    int plugin_match;
    
    if (!name || !name[0])
        return gui_current_window->buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->name)
        {
            plugin_match = 1;
            if (plugin && plugin[0])
            {
                if (ptr_buffer->plugin)
                {
                    if (strcmp (plugin, ptr_buffer->plugin->name) != 0)
                        plugin_match = 0;
                }
                else
                {
                    if (strcmp (plugin, "core") != 0)
                        plugin_match = 0;
                }
            }
            if (plugin_match && (strcmp (ptr_buffer->name, name) == 0))
            {
                return ptr_buffer;
            }
        }
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_buffer_search_by_partial_name: search a buffer by name (may be partial)
 */

struct t_gui_buffer *
gui_buffer_search_by_partial_name (const char *plugin, const char *name)
{
    struct t_gui_buffer *ptr_buffer, *buffer_partial_match;
    int plugin_match;
    
    if (!name || !name[0])
        return gui_current_window->buffer;
    
    buffer_partial_match = NULL;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->name)
        {
            plugin_match = 1;
            if (plugin && plugin[0])
            {
                if (ptr_buffer->plugin)
                {
                    if (strcmp (plugin, ptr_buffer->plugin->name) != 0)
                        plugin_match = 0;
                }
                else
                {
                    if (strcmp (plugin, "core") != 0)
                        plugin_match = 0;
                }
            }
            if (plugin_match)
            {
                if (strcmp (ptr_buffer->name, name) == 0)
                    return ptr_buffer;
                if (!buffer_partial_match && strstr (ptr_buffer->name, name))
                    buffer_partial_match = ptr_buffer;
            }
        }
    }
    
    /* return buffer partially matching (may be NULL if no buffer was found */
    return buffer_partial_match;
}

/*
 * gui_buffer_search_by_number: search a buffer by number
 */

struct t_gui_buffer *
gui_buffer_search_by_number (int number)
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == number)
            return ptr_buffer;
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_buffer_find_window: find a window displaying buffer
 */

struct t_gui_window *
gui_buffer_find_window (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    
    if (gui_current_window->buffer == buffer)
        return gui_current_window;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
            return ptr_win;
    }
    
    /* no window found */
    return NULL;
}

/*
 * gui_buffer_is_scrolled: return 1 if all windows displaying buffer are scrolled
 *                         (user doesn't see end of buffer)
 *                         return 0 if at least one window is NOT scrolled
 */

int
gui_buffer_is_scrolled (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    int buffer_found;

    if (!buffer)
        return 0;
    
    buffer_found = 0;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            buffer_found = 1;
            /* buffer found and not scrolled, exit immediately */
            if (!ptr_win->scroll)
                return 0;
        }
    }
    
    /* buffer found, and all windows were scrolled */
    if (buffer_found)
        return 1;
    
    /* buffer not found */
    return 0;
}

/*
 * gui_buffer_clear: clear buffer content
 */

void
gui_buffer_clear (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    
    if (!buffer)
        return;
    
    /* remove buffer from hotlist */
    gui_hotlist_remove_buffer (buffer);
    
    /* remove all lines */
    gui_chat_line_free_all (buffer);
    
    /* remove any scroll for buffer */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            ptr_win->first_line_displayed = 1;
            ptr_win->start_line = NULL;
            ptr_win->start_line_pos = 0;
        }
    }
    
    gui_buffer_ask_chat_refresh (buffer, 2);
    gui_status_refresh_needed = 1;
}

/*
 * gui_buffer_clear_all: clear all buffers content
 */

void
gui_buffer_clear_all ()
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_buffer_clear (ptr_buffer);
    }
}

/*
 * gui_buffer_close: close a buffer
 */

void
gui_buffer_close (struct t_gui_buffer *buffer, int switch_to_another)
{
    struct t_gui_window *ptr_window;
    struct t_gui_buffer *ptr_buffer;

    hook_signal_send ("buffer_closing",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    
    if (buffer->close_callback)
    {
        (void)(buffer->close_callback) (buffer->close_callback_data, buffer);
    }
    
    if (switch_to_another)
    {
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            if ((buffer == ptr_window->buffer) &&
                ((buffer->next_buffer) || (buffer->prev_buffer)))
                gui_buffer_switch_previous (ptr_window);
        }
    }
    
    gui_hotlist_remove_buffer (buffer);
    if (gui_hotlist_initial_buffer == buffer)
        gui_hotlist_initial_buffer = NULL;
    
    if (gui_previous_buffer == buffer)
        gui_previous_buffer = NULL;
    
    /* decrease buffer number for all next buffers */
    for (ptr_buffer = buffer->next_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number--;
    }
    
    /* free all lines */
    gui_chat_line_free_all (buffer);
    
    /* free some data */
    if (buffer->title)
        free (buffer->title);
    if (buffer->name)
        free (buffer->name);
    if (buffer->input_buffer)
        free (buffer->input_buffer);
    if (buffer->input_buffer_color_mask)
        free (buffer->input_buffer_color_mask);
    if (buffer->completion)
        gui_completion_free (buffer->completion);
    gui_history_buffer_free (buffer);
    if (buffer->text_search_input)
        free (buffer->text_search_input);
    gui_nicklist_remove_all (buffer);
    gui_nicklist_remove_group (buffer, buffer->nicklist_root);
    if (buffer->highlight_words)
        free (buffer->highlight_words);
    if (buffer->highlight_tags_array)
        string_free_exploded (buffer->highlight_tags_array);
    gui_keyboard_free_all (&buffer->keys, &buffer->last_key);
    
    /* remove buffer from buffers list */
    if (buffer->prev_buffer)
        buffer->prev_buffer->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        buffer->next_buffer->prev_buffer = buffer->prev_buffer;
    if (gui_buffers == buffer)
        gui_buffers = buffer->next_buffer;
    if (last_gui_buffer == buffer)
        last_gui_buffer = buffer->prev_buffer;
    
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer == buffer)
            ptr_window->buffer = NULL;
    }
    
    hook_signal_send ("buffer_closed",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    
    free (buffer);
    
    if (gui_windows && gui_current_window && gui_current_window->buffer)
        gui_status_refresh_needed = 1;
}

/*
 * gui_buffer_switch_previous: switch to previous buffer
 */

void
gui_buffer_switch_previous (struct t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if (window->buffer->prev_buffer)
        gui_window_switch_to_buffer (window, window->buffer->prev_buffer);
    else
        gui_window_switch_to_buffer (window, last_gui_buffer);
    
    gui_window_redraw_buffer (window->buffer);
}

/*
 * gui_buffer_switch_next: switch to next buffer
 */

void
gui_buffer_switch_next (struct t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if (window->buffer->next_buffer)
        gui_window_switch_to_buffer (window, window->buffer->next_buffer);
    else
        gui_window_switch_to_buffer (window, gui_buffers);
    
    gui_window_redraw_buffer (window->buffer);
}

/*
 * gui_buffer_switch_by_number: switch to another buffer with number
 */

void
gui_buffer_switch_by_number (struct t_gui_window *window, int number)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* invalid buffer */
    if (number < 0)
        return;
    
    /* buffer is currently displayed ? */
    if (number == window->buffer->number)
        return;
    
    /* search for buffer in the list */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer != window->buffer) && (number == ptr_buffer->number))
        {
            gui_window_switch_to_buffer (window, ptr_buffer);
            gui_window_redraw_buffer (window->buffer);
            return;
        }
    }
}

/*
 * gui_buffer_move_to_number: move a buffer to another number
 */

void
gui_buffer_move_to_number (struct t_gui_buffer *buffer, int number)
{
    struct t_gui_buffer *ptr_buffer;
    int i;
    char buf1_str[16], buf2_str[16], *argv[2] = { NULL, NULL };
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if (number < 1)
        number = 1;
    
    /* buffer number is already ok ? */
    if (number == buffer->number)
        return;
    
    snprintf (buf2_str, sizeof (buf2_str) - 1, "%d", buffer->number);
    
    /* remove buffer from list */
    if (buffer == gui_buffers)
    {
        gui_buffers = buffer->next_buffer;
        gui_buffers->prev_buffer = NULL;
    }
    if (buffer == last_gui_buffer)
    {
        last_gui_buffer = buffer->prev_buffer;
        last_gui_buffer->next_buffer = NULL;
    }
    if (buffer->prev_buffer)
        (buffer->prev_buffer)->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        (buffer->next_buffer)->prev_buffer = buffer->prev_buffer;
    
    if (number == 1)
    {
        gui_buffers->prev_buffer = buffer;
        buffer->prev_buffer = NULL;
        buffer->next_buffer = gui_buffers;
        gui_buffers = buffer;
    }
    else
    {
        /* assign new number to all buffers */
        i = 1;
        for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number = i++;
        }
        
        /* search for new position in the list */
        for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
        {
            if (ptr_buffer->number == number)
                break;
        }
        if (ptr_buffer)
        {
            /* insert before buffer found */
            buffer->prev_buffer = ptr_buffer->prev_buffer;
            buffer->next_buffer = ptr_buffer;
            if (ptr_buffer->prev_buffer)
                (ptr_buffer->prev_buffer)->next_buffer = buffer;
            ptr_buffer->prev_buffer = buffer;
        }
        else
        {
            /* number not found (too big)? => add to end */
            buffer->prev_buffer = last_gui_buffer;
            buffer->next_buffer = NULL;
            last_gui_buffer->next_buffer = buffer;
            last_gui_buffer = buffer;
        }
    }
    
    /* assign new number to all buffers */
    i = 1;
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number = i++;
    }
    
    gui_window_redraw_buffer (buffer);
    
    snprintf (buf1_str, sizeof (buf1_str) - 1, "%d", buffer->number);
    argv[0] = buf1_str;
    argv[1] = buf2_str;
    
    hook_signal_send ("buffer_moved",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * gui_buffer_add_to_infolist: add a buffer in an infolist
 *                             return 1 if ok, 0 if error
 */

int
gui_buffer_add_to_infolist (struct t_infolist *infolist,
                            struct t_gui_buffer *buffer)
{
    struct t_infolist_item *ptr_item;
    struct t_gui_key *ptr_key;
    char *pos_point, option_name[32];
    int i;
    
    if (!infolist || !buffer)
        return 0;
    
    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_pointer (ptr_item, "pointer", buffer))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "current_buffer",
                                   (gui_current_window->buffer == buffer) ? 1 : 0))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "plugin", buffer->plugin))
        return 0;
    if (!infolist_new_var_string (ptr_item, "plugin_name",
                                  (buffer->plugin) ?
                                  buffer->plugin->name : NULL))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "number", buffer->number))
        return 0;
    if (!infolist_new_var_string (ptr_item, "name", buffer->name))
        return 0;
    pos_point = strchr (buffer->name, '.');
    if (!infolist_new_var_string (ptr_item, "short_name",
                                  (pos_point) ? pos_point + 1 : buffer->name))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "type", buffer->type))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "notify", buffer->notify))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "num_displayed", buffer->num_displayed))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "lines_hidden", buffer->lines_hidden))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_case_sensitive", buffer->nicklist_case_sensitive))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_display_groups", buffer->nicklist_display_groups))
        return 0;
    if (!infolist_new_var_string (ptr_item, "title", buffer->title))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input", buffer->input))
        return 0;
    if (!infolist_new_var_string (ptr_item, "input_nick", buffer->input_nick))
        return 0;
    if (!infolist_new_var_string (ptr_item, "input_string", buffer->input_buffer))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_words", buffer->highlight_words))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_tags", buffer->highlight_tags))
        return 0;
    i = 0;
    for (ptr_key = buffer->keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        snprintf (option_name, sizeof (option_name), "key_%05d", i);
        if (!infolist_new_var_string (ptr_item, option_name, ptr_key->key))
            return 0;
        snprintf (option_name, sizeof (option_name), "key_command_%05d", i);
        if (!infolist_new_var_string (ptr_item, option_name, ptr_key->command))
            return 0;
    }
    
    return 1;
}

/*
 * gui_buffer_line_add_to_infolist: add a buffer line in an infolist
 *                                  return 1 if ok, 0 if error
 */

int
gui_buffer_line_add_to_infolist (struct t_infolist *infolist,
                                 struct t_gui_line *line)
{
    struct t_infolist_item *ptr_item;
    int i, length;
    char option_name[64], *tags;
    
    if (!infolist || !line)
        return 0;
    
    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!infolist_new_var_time (ptr_item, "date", line->date))
        return 0;
    if (!infolist_new_var_time (ptr_item, "date_printed", line->date))
        return 0;
    if (!infolist_new_var_string (ptr_item, "str_time", line->str_time))
        return 0;
    
    /* write tags */
    if (!infolist_new_var_integer (ptr_item, "tags_count", line->tags_count))
        return 0;
    for (i = 0; i < line->tags_count; i++)
    {
        snprintf (option_name, sizeof (option_name), "tag_%05d", i + 1);
        if (!infolist_new_var_string (ptr_item, option_name,
                                      line->tags_array[i]))
            return 0;
        length += strlen (line->tags_array[i]) + 1;
    }
    tags = malloc (length + 1);
    if (!tags)
        return 0;
    tags[0] = '\0';
    for (i = 0; i < line->tags_count; i++)
    {
        strcat (tags, line->tags_array[i]);
        if (i < line->tags_count - 1)
            strcat (tags, ",");
    }
    if (!infolist_new_var_string (ptr_item, "tags", tags))
    {
        free (tags);
        return 0;
    }
    free (tags);
    
    if (!infolist_new_var_integer (ptr_item, "displayed", line->displayed))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "highlight", line->highlight))
        return 0;
    if (!infolist_new_var_string (ptr_item, "prefix", line->prefix))
        return 0;
    if (!infolist_new_var_string (ptr_item, "message", line->message))
        return 0;
    
    return 1;
}

/*
 * gui_buffer_dump_hexa: dump content of buffer as hexa data in log file
 */

void
gui_buffer_dump_hexa (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line;
    int num_line, msg_pos;
    char *message_without_colors;
    char hexa[(16 * 3) + 1], ascii[(16 * 2) + 1];
    int hexa_pos, ascii_pos;
    
    log_printf ("[buffer dump hexa (addr:0x%x)]", buffer);
    num_line = 1;
    for (ptr_line = buffer->lines; ptr_line; ptr_line = ptr_line->next_line)
    {
        /* display line without colors */
        message_without_colors = (ptr_line->message) ?
            (char *)gui_color_decode ((unsigned char *)ptr_line->message) : NULL;
        log_printf ("");
        log_printf ("  line %d: %s",
                    num_line,
                    (message_without_colors) ?
                    message_without_colors : "(null)");
        if (message_without_colors)
            free (message_without_colors);

        /* display raw message for line */
        if (ptr_line->message)
        {
            log_printf ("");
            log_printf ("  raw data for line %d (with color codes):",
                        num_line);
            msg_pos = 0;
            hexa_pos = 0;
            ascii_pos = 0;
            while (ptr_line->message[msg_pos])
            {
                snprintf (hexa + hexa_pos, 4, "%02X ",
                          (unsigned char)(ptr_line->message[msg_pos]));
                hexa_pos += 3;
                snprintf (ascii + ascii_pos, 3, "%c ",
                          ((((unsigned char)ptr_line->message[msg_pos]) < 32)
                           || (((unsigned char)ptr_line->message[msg_pos]) > 127)) ?
                          '.' : (unsigned char)(ptr_line->message[msg_pos]));
                ascii_pos += 2;
                if (ascii_pos == 32)
                {
                    log_printf ("    %-48s  %s", hexa, ascii);
                    hexa_pos = 0;
                    ascii_pos = 0;
                }
                msg_pos++;
            }
            if (ascii_pos > 0)
                log_printf ("    %-48s  %s", hexa, ascii);
        }
        
        num_line++;
    }
}

/*
 * gui_buffer_print_log: print buffer infos in log (usually for crash dump)
 */

void
gui_buffer_print_log ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    char *tags;
    int num;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        log_printf ("");
        log_printf ("[buffer (addr:0x%x)]", ptr_buffer);
        log_printf ("  plugin . . . . . . . . : 0x%x", ptr_buffer->plugin);
        log_printf ("  number . . . . . . . . : %d",   ptr_buffer->number);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_buffer->name);
        log_printf ("  type . . . . . . . . . : %d",   ptr_buffer->type);
        log_printf ("  notify . . . . . . . . : %d",   ptr_buffer->notify);
        log_printf ("  num_displayed. . . . . : %d",   ptr_buffer->num_displayed);
        log_printf ("  title. . . . . . . . . : '%s'", ptr_buffer->title);
        log_printf ("  lines. . . . . . . . . : 0x%x", ptr_buffer->lines);
        log_printf ("  last_line. . . . . . . : 0x%x", ptr_buffer->last_line);
        log_printf ("  last_read_line . . . . : 0x%x", ptr_buffer->last_read_line);
        log_printf ("  lines_count. . . . . . : %d",   ptr_buffer->lines_count);
        log_printf ("  lines_hidden . . . . . : %d",   ptr_buffer->lines_hidden);
        log_printf ("  prefix_max_length. . . : %d",   ptr_buffer->prefix_max_length);
        log_printf ("  chat_refresh_needed. . : %d",   ptr_buffer->chat_refresh_needed);
        log_printf ("  nicklist . . . . . . . : %d",   ptr_buffer->nicklist);
        log_printf ("  nicklist_case_sensitive: %d",   ptr_buffer->nicklist_case_sensitive);
        log_printf ("  nicklist_root. . . . . : 0x%x", ptr_buffer->nicklist_root);
        log_printf ("  nicklist_max_length. . : %d",   ptr_buffer->nicklist_max_length);
        log_printf ("  nicklist_display_groups: %d",   ptr_buffer->nicklist_display_groups);
        log_printf ("  nicklist_visible_count.: %d",   ptr_buffer->nicklist_visible_count);
        log_printf ("  nicklist_refresh_needed: %d",   ptr_buffer->nicklist_refresh_needed);
        log_printf ("  input. . . . . . . . . : %d",   ptr_buffer->input);
        log_printf ("  input_callback . . . . : 0x%x", ptr_buffer->input_callback);
        log_printf ("  input_callback_data. . : 0x%x", ptr_buffer->input_callback_data);
        log_printf ("  input_nick . . . . . . : '%s'", ptr_buffer->input_nick);
        log_printf ("  input_buffer . . . . . : '%s'", ptr_buffer->input_buffer);
        log_printf ("  input_buffer_color_mask: '%s'", ptr_buffer->input_buffer_color_mask);
        log_printf ("  input_buffer_alloc . . : %d",   ptr_buffer->input_buffer_alloc);
        log_printf ("  input_buffer_size. . . : %d",   ptr_buffer->input_buffer_size);
        log_printf ("  input_buffer_length. . : %d",   ptr_buffer->input_buffer_length);
        log_printf ("  input_buffer_pos . . . : %d",   ptr_buffer->input_buffer_pos);
        log_printf ("  input_buffer_1st_disp. : %d",   ptr_buffer->input_buffer_1st_display);
        log_printf ("  completion . . . . . . : 0x%x", ptr_buffer->completion);
        log_printf ("  history. . . . . . . . : 0x%x", ptr_buffer->history);
        log_printf ("  last_history . . . . . : 0x%x", ptr_buffer->last_history);
        log_printf ("  ptr_history. . . . . . : 0x%x", ptr_buffer->ptr_history);
        log_printf ("  num_history. . . . . . : %d",   ptr_buffer->num_history);
        log_printf ("  text_search. . . . . . : %d",   ptr_buffer->text_search);
        log_printf ("  text_search_exact. . . : %d",   ptr_buffer->text_search_exact);
        log_printf ("  text_search_found. . . : %d",   ptr_buffer->text_search_found);
        log_printf ("  text_search_input. . . : '%s'", ptr_buffer->text_search_input);
        log_printf ("  highlight_words. . . . : '%s'", ptr_buffer->highlight_words);
        log_printf ("  highlight_tags_count . : %d",   ptr_buffer->highlight_tags_count);
        log_printf ("  highlight_tags_array . : 0x%x", ptr_buffer->highlight_tags_array);
        log_printf ("  prev_buffer. . . . . . : 0x%x", ptr_buffer->prev_buffer);
        log_printf ("  next_buffer. . . . . . : 0x%x", ptr_buffer->next_buffer);

        log_printf ("");
        log_printf ("  => nicklist_root (addr:0x%x):", ptr_buffer->nicklist_root);
        gui_nicklist_print_log (ptr_buffer->nicklist_root, 0);

        if (ptr_buffer->keys)
        {
            log_printf ("");
            log_printf ("  => keys = 0x%x, last_key = 0x%x:",
                        ptr_buffer->keys, ptr_buffer->last_key);
            gui_keyboard_print_log (ptr_buffer);
        }
        
        log_printf ("");
        log_printf ("  => last 100 lines:");
        num = 0;
        ptr_line = ptr_buffer->last_line;
        while (ptr_line && (num < 100))
        {
            num++;
            ptr_line = ptr_line->prev_line;
        }
        if (!ptr_line)
            ptr_line = ptr_buffer->lines;
        else
            ptr_line = ptr_line->next_line;
        
        while (ptr_line)
        {
            num--;
            tags = string_build_with_exploded (ptr_line->tags_array, ",");
            log_printf ("       line N-%05d: y:%d, str_time:'%s', tags:'%s', "
                        "displayed:%d, highlight:%d, refresh_needed:%d, prefix:'%s'",
                        num, ptr_line->y, ptr_line->str_time,
                        (tags) ? tags  : "",
                        (int)(ptr_line->displayed),
                        (int) (ptr_line->highlight),
                        (int)(ptr_line->refresh_needed),
                        ptr_line->prefix);
            log_printf ("                     data: '%s'",
                        ptr_line->message);
            if (tags)
                free (tags);
            
            ptr_line = ptr_line->next_line;
        }
        
        if (ptr_buffer->completion)
        {
            log_printf ("");
            gui_completion_print_log (ptr_buffer->completion);
        }
    }
}
