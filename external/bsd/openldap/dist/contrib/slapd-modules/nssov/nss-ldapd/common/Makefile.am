# Makefile.am - use automake to generate Makefile.in
#
# Copyright (C) 2007, 2008 Arthur de Jong
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA

noinst_LIBRARIES = libtio.a libdict.a

AM_CPPFLAGS=-I$(top_srcdir)
AM_CFLAGS = -fPIC

libtio_a_SOURCES = tio.c tio.h

libdict_a_SOURCES = dict.c dict.h \
                    set.c set.h
