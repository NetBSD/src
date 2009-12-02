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
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

srcdir = .
top_srcdir = ../..
top_builddir = ../..


ifeq ("no", "yes")
	DEFS += -DDEBUG
endif

TARGETS = vgtest
test_SOURCES = test.c
vgtest_SOURCES = vgtest.c
INCLUDES += -I../../include

LVMLIBS =  -ldevmapper
DEPLIBS = $(top_srcdir)/liblvm/liblvm2app.so $(top_srcdir)/libdm/libdevmapper.so

DEFS += -D_REENTRANT

include $(top_srcdir)/make.tmpl

LDFLAGS = -L$(top_srcdir)/libdm -L$(top_srcdir)/liblvm

ifeq ("", "yes")
	LVMLIBS += -ldevmapper-event -lpthread
	LDFLAGS += -L$(top_srcdir)/daemons/dmeventd
endif

test_OBJECTS = $(test_SOURCES:.c=.o)
vgtest_OBJECTS = $(vgtest_SOURCES:.c=.o)
OBJECTS = $(test_OBJECTS) $(vgtest_OBJECTS)

test: $(test_OBJECTS) $(DEPLIBS)
	$(CC) -o test $(test_OBJECTS) $(CFLAGS) $(LDFLAGS) $(LVMLIBS) $(LIBS)

vgtest: $(vgtest_OBJECTS) $(DEPLIBS)
	$(CC) -o vgtest $(vgtest_OBJECTS) $(CFLAGS) $(LDFLAGS) $(LVMLIBS) $(LIBS)
