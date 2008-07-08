# Makefile.am - use automake to generate Makefile.in
#
# Copyright (C) 2006 West Consulting
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

TESTS = test_dict test_set test_tio test_cfg test_myldap.sh test_nsscmds.sh

check_PROGRAMS = test_dict test_set test_tio test_cfg test_myldap

EXTRA_PROGRAMS = test_aliases test_ethers test_group test_hosts \
                 test_netgroup test_networks test_passwd test_protocols \
                 test_rpc test_services test_shadow \
                 test_nslcd_group

EXTRA_DIST = nss-ldapd-test.conf test_myldap.sh test_nsscmds.sh usernames.txt

CLEANFILES = $(EXTRA_PROGRAMS)

AM_CPPFLAGS = -I$(top_srcdir)
AM_CFLAGS = $(PTHREAD_CFLAGS) -g

# the following enables verbose protocol debugging information to be dumped
#AM_CPPFLAGS += -DDEBUG_PROT -DDEBUG_PROT_DUMP

# the following enabled verbose tio stats logging
#AM_CPPFLAGS += -DDEBUG_TIO_STATS

test_dict_SOURCES = test_dict.c ../common/dict.h ../common/dict.c
#test_dict_LDADD = ../common/dict.o

test_set_SOURCES = test_set.c ../common/set.h
test_set_LDADD = ../common/libdict.a

test_tio_SOURCES = test_tio.c ../common/tio.h ../common/tio.c
test_tio_LDFLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)

test_cfg_SOURCES = test_cfg.c
test_cfg_LDADD = ../nslcd/log.o ../nslcd/attmap.o \
                 ../nslcd/common.o ../nslcd/myldap.o \
                 ../nslcd/alias.o ../nslcd/ether.o ../nslcd/group.o \
                 ../nslcd/host.o ../nslcd/netgroup.o ../nslcd/network.o \
                 ../nslcd/passwd.o ../nslcd/protocol.o ../nslcd/rpc.o \
                 ../nslcd/service.o ../nslcd/shadow.o \
                 @nslcd_LIBS@ ../common/libtio.a ../common/libdict.a \
                 ../compat/libcompat.a

test_myldap_SOURCES = test_myldap.c
test_myldap_LDADD = ../nslcd/log.o ../nslcd/common.o ../nslcd/cfg.o \
                    ../nslcd/myldap.o @nslcd_LIBS@ ../common/libtio.a \
                    ../common/libdict.a ../compat/libcompat.a

common_SOURCES = ../nss/common.c ../nslcd.h ../nss/prototypes.h \
                 ../common/tio.c ../common/tio.h

test_aliases_SOURCES = test_aliases.c ../nss/aliases.c $(common_SOURCES)
test_ethers_SOURCES = test_ethers.c ../nss/ethers.c $(common_SOURCES)
test_group_SOURCES = test_group.c ../nss/group.c $(common_SOURCES)
test_hosts_SOURCES = test_hosts.c ../nss/hosts.c $(common_SOURCES)
test_netgroup_SOURCES = test_netgroup.c ../nss/netgroup.c $(common_SOURCES)
test_networks_SOURCES = test_networks.c ../nss/networks.c $(common_SOURCES)
test_passwd_SOURCES = test_passwd.c ../nss/passwd.c $(common_SOURCES)
test_protocols_SOURCES = test_protocols.c ../nss/protocols.c $(common_SOURCES)
test_rpc_SOURCES = test_rpc.c ../nss/rpc.c $(common_SOURCES)
test_services_SOURCES = test_services.c ../nss/services.c $(common_SOURCES)
test_shadow_SOURCES = test_shadow.c ../nss/shadow.c $(common_SOURCES)

test_nslcd_group_SOURCES = test_nslcd_group.c
test_nslcd_group_LDADD = ../nslcd/log.o ../nslcd/attmap.o ../common/dict.o \
                         ../nslcd/common.o ../nslcd/myldap.o \
                         ../nslcd/alias.o ../nslcd/ether.o \
                         ../nslcd/host.o ../nslcd/netgroup.o ../nslcd/network.o \
                         ../nslcd/passwd.o ../nslcd/protocol.o ../nslcd/rpc.o \
                         ../nslcd/service.o ../nslcd/shadow.o \
                         @nslcd_LIBS@ ../common/libtio.a ../common/libdict.a \
                         ../compat/libcompat.a ../nslcd/cfg.o
