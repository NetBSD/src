#
# Copyright (C) 2005-2007 Red Hat, Inc. All rights reserved.
#
# This file is part of the device-mapper userspace tools.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU Lesser General Public License v.2.1.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

srcdir = .
top_srcdir = ../..


SOURCES = libdevmapper-event.c

LIB_STATIC = libdevmapper-event.a
LIB_VERSION = $(LIB_VERSION_DM)

ifeq ("","dylib")
  LIB_SHARED = libdevmapper-event.dylib
else
  LIB_SHARED = libdevmapper-event.so
  VERSIONED_SHLIB = $(LIB_SHARED).$(LIB_VERSION)
endif

TARGETS = dmeventd
CLEAN_TARGETS = dmeventd.o

ifneq ($(MAKECMDGOALS),device-mapper)
  SUBDIRS+=plugins
endif

include $(top_srcdir)/make.tmpl

all: dmeventd
device-mapper: dmeventd

LDFLAGS += -ldl -ldevmapper -lpthread
CLDFLAGS += -ldl -ldevmapper -lpthread

dmeventd: $(LIB_SHARED) $(VERSIONED_SHLIB) dmeventd.o
	$(CC) -o $@ dmeventd.o $(CFLAGS) $(LDFLAGS) \
	-L. -ldevmapper-event $(LIBS) -rdynamic

.PHONY: install_dynamic install_static install_include \
	install_pkgconfig install_dmeventd

INSTALL_TYPE = install_dynamic

ifeq ("no", "yes")
  INSTALL_TYPE += install_static
endif

ifeq ("no", "yes")
  INSTALL_TYPE += install_pkgconfig
endif

install: $(INSTALL_TYPE) install_include install_dmeventd

install_device-mapper: install

install_include:
	$(INSTALL) -D $(OWNER) $(GROUP) -m 444 libdevmapper-event.h \
		$(includedir)/libdevmapper-event.h

install_dynamic: libdevmapper-event.$(LIB_SUFFIX)
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< \
		$(libdir)/libdevmapper-event.$(LIB_SUFFIX).$(LIB_VERSION)
	$(LN_S) -f libdevmapper-event.$(LIB_SUFFIX).$(LIB_VERSION) \
		$(libdir)/libdevmapper-event.$(LIB_SUFFIX)

install_dmeventd: dmeventd
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< $(sbindir)/$<

install_pkgconfig:
	$(INSTALL) -D $(OWNER) $(GROUP) -m 444 libdevmapper-event.pc \
		$(usrlibdir)/pkgconfig/devmapper-event.pc

install_static: libdevmapper-event.a
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< \
		$(libdir)/libdevmapper-event.a.$(LIB_VERSION)
	$(LN_S) -f libdevmapper-event.a.$(LIB_VERSION) $(libdir)/libdevmapper-event.a

$(VERSIONED_SHLIB): $(LIB_SHARED)
	$(RM) -f $@
	$(LN_S) $(LIB_SHARED) $@

.PHONY: distclean_lib distclean

distclean_lib: 
	$(RM) libdevmapper-event.pc

distclean: distclean_lib

