#
# Copyright (C) 2014 Andrew Potter <agpotter@gmail.com>
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(DBUS_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = dbus.la

dbus_la_SOURCES = dbus.c                           \
                  dbus-mainloop.c                  \
                  dbus-mainloop.h                  \
                  dbus-argument.c                  \
                  dbus-argument.h                  \
                  dbus-property.c                  \
                  dbus-property.h                  \
                  dbus-signal.c                    \
                  dbus-signal.h                    \
                  dbus-method.c                    \
                  dbus-method.h                    \
                  dbus-interface.c                 \
                  dbus-interface.h                 \
                  dbus-object.c                    \
                  dbus-object.h                    \
                  dbus-object-factory.c            \
                  dbus-object-factory.h            \
                  dbus-interfaces-peer.c           \
                  dbus-interfaces-peer.h           \
                  dbus-interfaces-introspectable.c \
                  dbus-interfaces-introspectable.h \
                  dbus-interfaces-properties.c     \
                  dbus-interfaces-properties.h     \
                  dbus-interfaces-object-manager.c \
                  dbus-interfaces-object-manager.h \
                  dbus-interfaces-buffer.c         \
                  dbus-interfaces-buffer.h         \
                  dbus-strings.c                   \
                  dbus-strings.h                   \
                  dbus-infolist.c                  \
                  dbus-infolist.h                  \
                  dbus-methods-core.c              \
                  dbus-methods-core.h              \
                  dbus.h

dbus_la_LDFLAGS = -module -no-undefined
dbus_la_LIBADD  = $(DBUS_LFLAGS)

EXTRA_DIST = CMakeLists.txt
