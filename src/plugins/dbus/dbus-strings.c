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

#include "dbus-strings.h"

const char WEECHAT_DBUS_NAME[]                                      = "org.weechat";
const char WEECHAT_DBUS_OBJECT_ROOT[]                               = "/org/weechat";

/* Signals */
const char WEECHAT_DBUS_OBJECT_SIGNAL[]                             = "/org/weechat/signal";
const char WEECHAT_DBUS_OBJECT_SIGNAL_RELATIVE[]                    = "signal";

const char WEECHAT_DBUS_IFACE_SIGNAL_IRC[]                          = "org.weechat.signal.irc";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_IN[]                      = "in2";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_IN[]                  = "sss";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_OUT[]                     = "out1";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_OUT[]                 = "sss";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_CTCP[]                    = "ctcp";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_CTCP[]                = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_DCC[]                     = "dcc";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_DCC[]                 = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_PV[]                      = "pv";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_PV[]                  = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_CONNECTING[]       = "serverConnecting";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_SERVER_CONNECTING[]   = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_CONNECTED[]        = "serverConnected";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_SERVER_CONNECTED[]    = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_SERVER_DISCONNECTED[]     = "serverDisconnected";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_SERVER_DISCONNECTED[] = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_JOIN[]             = "notifyJoin";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_NOTIFY_JOIN[]         = "as";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_QUIT[]             = "notifyQuit";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_NOTIFY_QUIT[]         = "as";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_AWAY[]             = "notifyAway";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_NOTIFY_AWAY[]         = "as";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_STILL_AWAY[]       = "notifyStillAway";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_NOTIFY_STILL_AWAY[]   = "as";
const char WEECHAT_DBUS_MEMBER_SIGNAL_IRC_NOTIFY_BACK[]             = "notifyBack";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_IRC_NOTIFY_BACK[]         = "as";

const char WEECHAT_DBUS_IFACE_SIGNAL_CORE[]                         = "org.weechat.signal.core";
const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_DAY_CHANGED[]            = "dayChanged";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_CORE_DAY_CHANGED[]        = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PLUGIN_LOADED[]          = "pluginLoaded";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_CORE_PLUGIN_LOADED[]      = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PLUGIN_UNLOADED[]        = "pluginUnloaded";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_CORE_PLUGIN_UNLOADED[]    = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_QUIT[]                   = "quit";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_CORE_QUIT[]               = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_UPGRADE[]                = "upgrade";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_CORE_UPGRADE[]            = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_HIGHLIGHT[]              = "highlight";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_CORE_HIGHLIGHT[]          = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_CORE_PV[]                     = "pv";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_CORE_PV[]                 = "s";

const char WEECHAT_DBUS_IFACE_SIGNAL_XFER[]                         = "org.weechat.signal.xfer";
const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ADD[]                    = "add";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_XFER_ADD[]                = "{sv}";
const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_SEND_READY[]             = "sendReady";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_XFER_SEND_READY[]         = "{sv}";
const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ACCEPT_RESUME[]          = "acceptResume";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_XFER_ACCEPT_RESUME[]      = "{sv}";
const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_SEND_ACCEPT_RESUME[]     = "sendAcceptResume";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_XFER_SEND_ACCEPT_RESUME[] = "{sv}";
const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_START_RESUME[]           = "startResume";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_XFER_START_RESUME[]       = "{sv}";
const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_RESUME_READY[]           = "resumeReady";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_XFER_RESUME_READY[]       = "{sv}";
const char WEECHAT_DBUS_MEMBER_SIGNAL_XFER_ENDED[]                  = "ended";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_XFER_ENDED[]              = "{sv}";

const char WEECHAT_DBUS_IFACE_SIGNAL_GUILE[]                        = "org.weechat.signal.guile";
const char WEECHAT_DBUS_IFACE_SIGNAL_LUA[]                          = "org.weechat.signal.lua";
const char WEECHAT_DBUS_IFACE_SIGNAL_PERL[]                         = "org.weechat.signal.perl";
const char WEECHAT_DBUS_IFACE_SIGNAL_PYTHON[]                       = "org.weechat.signal.python";
const char WEECHAT_DBUS_IFACE_SIGNAL_RUBY[]                         = "org.weechat.signal.ruby";
const char WEECHAT_DBUS_IFACE_SIGNAL_TCL[]                          = "org.weechat.signal.tcl";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_LOADED[]               = "scriptLoaded";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_SCRIPT_LOADED[]           = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_UNLOADED[]             = "scriptUnloaded";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_SCRIPT_UNLOADED[]         = "s";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_INSTALLED[]            = "scriptInstalled";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_SCRIPT_INSTALLED[]        = "as";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SCRIPT_REMOVED[]              = "scriptRemoved";
const char WEECHAT_DBUS_MEMBER_SIGNAL_SIG_SCRIPT_REMOVED[]          = "as";

/* Methods */
const char WEECHAT_DBUS_OBJECT_CORE[]                               = "/org/weechat/core";
const char WEECHAT_DBUS_OBJECT_CORE_RELATIVE[]                      = "core";
const char WEECHAT_DBUS_IFACE_CORE[]                                = "org.weechat.core";
const char WEECHAT_DBUS_MEMBER_CORE_INFOGET[]                       = "infoGet";
const char WEECHAT_DBUS_MEMBER_CORE_SIGIN_INFOGET[]                 = "ss";
const char WEECHAT_DBUS_MEMBER_CORE_SIGOUT_INFOGET[]                = "s";
const char WEECHAT_DBUS_MEMBER_CORE_COMMAND[]                       = "command";
const char WEECHAT_DBUS_MEMBER_CORE_SIGIN_COMMAND[]                 = "ss";
const char WEECHAT_DBUS_MEMBER_CORE_SIGOUT_COMMAND[]                = "";
const char WEECHAT_DBUS_MEMBER_CORE_GET_BUFFERS[]                   = "getBuffers";
const char WEECHAT_DBUS_MEMBER_CORE_SIGIN_GET_BUFFERS[]             = "";
const char WEECHAT_DBUS_MEMBER_CORE_SIGOUT_GET_BUFFERS[]            = "as";


