#
# Copyright (C) 2009 Red Hat, Inc. All rights reserved.
#
# This file is part of LVM2.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

srcdir = .
top_srcdir = ../..
top_builddir = ../..


CPG_LIBS = 
CPG_CFLAGS = 
SACKPT_LIBS = 
SACKPT_CFLAGS = 

SOURCES = clogd.c cluster.c functions.c link_mon.c local.c logging.c

TARGETS = cmirrord

include ../../make.tmpl

LIBS += -ldevmapper
LMLIBS += $(CPG_LIBS) $(SACKPT_LIBS)
CFLAGS += $(CPG_CFLAGS) $(SACKPT_CFLAGS)

cmirrord: $(OBJECTS) $(top_builddir)/lib/liblvm-internal.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o cmirrord $(OBJECTS) \
		$(LVMLIBS) $(LMLIBS) $(LIBS)

install: $(TARGETS)
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) cmirrord \
		$(usrsbindir)/cmirrord
