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
top_builddir = ../..


SOURCES = libdevmapper-event.c
TARGETS = dmeventd

.PHONY: install_lib_dynamic install_lib_static install_include \
	install_pkgconfig install_dmeventd_dynamic install_dmeventd_static \
	install_lib install_dmeventd

INSTALL_DMEVENTD_TARGETS = install_dmeventd_dynamic
INSTALL_LIB_TARGETS = install_lib_dynamic

ifeq ("no", "yes")
  LIB_STATIC = libdevmapper-event.a
  TARGETS += dmeventd.static
  INSTALL_DMEVENTD_TARGETS += install_dmeventd_static
  INSTALL_LIB_TARGETS += install_lib_static
endif

LIB_VERSION = $(LIB_VERSION_DM)

ifeq ("","dylib")
  LIB_SHARED = libdevmapper-event.dylib
else
  LIB_SHARED = libdevmapper-event.so
  VERSIONED_SHLIB = $(LIB_SHARED).$(LIB_VERSION)
endif

CLEAN_TARGETS = dmeventd.o dmeventd.static

ifneq ($(MAKECMDGOALS),device-mapper)
  SUBDIRS+=plugins
endif

include ../../make.tmpl

all: device-mapper
device-mapper: $(TARGETS)

LIBS += -ldl
LVMLIBS += -ldevmapper-event -lpthread -ldevmapper

$(VERSIONED_SHLIB): $(LIB_SHARED)
	$(RM) -f $@
	$(LN_S) $(LIB_SHARED) $@

dmeventd: $(LIB_SHARED) $(VERSIONED_SHLIB) dmeventd.o
	$(CC) $(CFLAGS) $(LDFLAGS) -L. -o $@ dmeventd.o \
	$(LVMLIBS) $(LIBS) -rdynamic

dmeventd.static: $(LIB_STATIC) dmeventd.o $(interfacedir)/libdevmapper.a
	$(CC) $(CFLAGS) $(LDFLAGS) -static -L. -L$(interfacedir) -o $@ \
	dmeventd.o $(LVMLIBS) $(LIBS) -rdynamic

ifeq ("no", "yes")
  INSTALL_LIB_TARGETS += install_pkgconfig
endif

install_include:
	$(INSTALL) -D $(OWNER) $(GROUP) -m 444 $(srcdir)/libdevmapper-event.h \
		$(includedir)/libdevmapper-event.h

install_pkgconfig:
	$(INSTALL) -D $(OWNER) $(GROUP) -m 444 libdevmapper-event.pc \
		$(usrlibdir)/pkgconfig/devmapper-event.pc

install_lib_dynamic: libdevmapper-event.$(LIB_SUFFIX)
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< \
		$(libdir)/libdevmapper-event.$(LIB_SUFFIX).$(LIB_VERSION)
	$(LN_S) -f libdevmapper-event.$(LIB_SUFFIX).$(LIB_VERSION) \
		$(libdir)/libdevmapper-event.$(LIB_SUFFIX)

install_lib_static: libdevmapper-event.a
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< \
		$(libdir)/libdevmapper-event.a.$(LIB_VERSION)
	$(LN_S) -f libdevmapper-event.a.$(LIB_VERSION) $(libdir)/libdevmapper-event.a

install_lib: $(INSTALL_LIB_TARGETS)

install_dmeventd_dynamic: dmeventd
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< $(sbindir)/$<

install_dmeventd_static: dmeventd.static
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< $(sbindir)/$<

install_dmeventd: $(INSTALL_DMEVENTD_TARGETS)

install: install_include install_lib install_dmeventd

install_device-mapper: install_include install_lib install_dmeventd

.PHONY: distclean_lib distclean

distclean_lib: 
	$(RM) libdevmapper-event.pc

distclean: distclean_lib

