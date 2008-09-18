# Makefile.am - use automake to generate Makefile.in
#
# Copyright (C) 2006, 2007 West Consulting
# Copyright (C) 2006, 2007, 2008 Arthur de Jong
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

sbin_PROGRAMS = nslcd

AM_CPPFLAGS=-I$(top_srcdir)
AM_CFLAGS = $(PTHREAD_CFLAGS)

nslcd_SOURCES = nslcd.c ../nslcd.h ../nslcd-common.h \
                ../compat/attrs.h \
                log.c log.h \
                common.c common.h \
                myldap.c myldap.h \
                cfg.c cfg.h \
                attmap.c attmap.h \
                alias.c ether.c group.c host.c netgroup.c network.c \
                passwd.c protocol.c rpc.c service.c shadow.c
nslcd_LDADD = @nslcd_LIBS@ ../common/libtio.a ../common/libdict.a \
              ../compat/libcompat.a
