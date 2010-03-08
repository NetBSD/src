# Makefile.am - use automake to generate Makefile.in
#
# Copyright (C) 2006 Luke Howard
# Copyright (C) 2006 West Consulting
# Copyright (C) 2006, 2007 Arthur de Jong
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

noinst_PROGRAMS = nss_ldap.so

AM_CPPFLAGS=-I$(top_srcdir)
AM_CFLAGS = -fPIC

# determin version numbers to use in installed files
NSS_VERS = 2
NSS_LDAP_NSS_VERSIONED = libnss_ldap.so.$(NSS_VERS)

nss_ldap_so_SOURCES = common.c common.h prototypes.h \
                      ../nslcd.h ../nslcd-common.h \
                      ../compat/attrs.h \
                      aliases.c ethers.c group.c hosts.c netgroup.c \
                      networks.c passwd.c protocols.c rpc.c services.c \
                      shadow.c pam.c
nss_ldap_so_LDFLAGS = -shared -Wl,-soname,$(NSS_LDAP_NSS_VERSIONED) \
                      -Wl,--version-script,\$(srcdir)/exports.linux
nss_ldap_so_LDADD = @nss_ldap_so_LIBS@ ../common/libtio.a -lpam

EXTRA_DIST = exports.linux

install-exec-local: install-nss_ldap_so
uninstall-local: uninstall-nss_ldap_so

# install libnss_ldap.so.2
install-nss_ldap_so: nss_ldap.so
	$(INSTALL_PROGRAM) -D nss_ldap.so $(DESTDIR)$(libdir)/$(NSS_LDAP_NSS_VERSIONED)
uninstall-nss_ldap_so:
	-rm -f $(DESTDIR)$(libdir)/$(NSS_LDAP_NSS_VERSIONED)
