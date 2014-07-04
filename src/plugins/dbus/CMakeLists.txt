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

ADD_LIBRARY(dbus MODULE
dbus.c dbus.h
dbus-mainloop.c dbus-mainloop.h
dbus-infolist.c dbus-infolist.h
dbus-strings.c dbus-strings.h
dbus-argument.c dbus-argument.h
dbus-property.c dbus-property.h
dbus-signal.c dbus-signal.h
dbus-method.c dbus-method.h
dbus-interface.c dbus-interface.h
dbus-object.c dbus-object.h
dbus-object-factory.c dbus-object-factory.h
dbus-interfaces-peer.c dbus-interfaces-peer.h
dbus-interfaces-introspectable.c dbus-interfaces-introspectable.h
dbus-interfaces-properties.c dbus-interfaces-properties.h
dbus-interfaces-object-manager.c dbus-interfaces-object-manager.h
dbus-interfaces-buffer.c dbus-interfaces-buffer.h
dbus-methods-core.c dbus-methods-core.h
)

SET_TARGET_PROPERTIES(dbus PROPERTIES PREFIX "")

IF(DBUS_FOUND)
  INCLUDE_DIRECTORIES(${DBUS_INCLUDE_DIR} ${DBUS_ARCH_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})
  SET(LINK_LIBS)
  LIST(APPEND LINK_LIBS ${DBUS_LDFLAGS} ${LIBXML2_LDFLAGS})
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${DBUS_LFLAGS} ${LIBXML2_LFLAGS}")
  TARGET_LINK_LIBRARIES(dbus ${LINK_LIBS} ${DBUS_LIBRARIES} ${LIBXML2_LIBRARIES})
ENDIF(DBUS_FOUND)

INSTALL(TARGETS dbus LIBRARY DESTINATION ${LIBDIR}/plugins)
