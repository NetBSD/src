#
# Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
# Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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
top_srcdir = ..
top_builddir = ..


SOURCES =\
	lvm_base.c \
	lvm_lv.c \
	lvm_pv.c \
	lvm_vg.c

LIB_NAME = liblvm2app
LIB_VERSION = $(LIB_VERSION_APP)
VERSIONED_SHLIB = $(LIB_NAME).$(LIB_SUFFIX).$(LIB_VERSION_APP)

ifeq ("no", "yes")
  LIB_STATIC = $(LIB_NAME).a
endif

ifeq ("","dylib")
  LIB_SHARED = $(LIB_NAME).dylib
else
  LIB_SHARED = $(LIB_NAME).so
endif

CLEAN_TARGETS += liblvm.cflow

include ../make.tmpl

LIBS += -ldevmapper -llvm-internal

ifeq ("", "yes")
  LIBS += -ldevmapper-event
endif

$(VERSIONED_SHLIB): %.$(LIB_SUFFIX).$(LIB_VERSION_APP): %.$(LIB_SUFFIX)
	rm -f $@
	$(LN_S) $< $@

.PHONY: install_dynamic install_static install_include install_pkgconfig \
	distclean_lib distclean

INSTALL_TYPE = install_dynamic

ifeq ("no", "yes")
  INSTALL_TYPE += install_static
endif

ifeq ("no", "yes")
  INSTALL_TYPE += install_pkgconfig
endif

install: $(INSTALL_TYPE) install_include

install_include:
	$(INSTALL) -D $(OWNER) $(GROUP) -m 444 $(srcdir)/lvm2app.h $(includedir)/lvm2app.h

install_dynamic: $(LIB_SHARED)
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< \
		$(libdir)/$(LIB_SHARED).$(LIB_VERSION_APP)
	$(LN_S) -f $(LIB_SHARED).$(LIB_VERSION_APP) \
		$(libdir)/$(LIB_SHARED)

install_static: $(LIB_STATIC)
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< \
		$(libdir)/$(LIB_STATIC).$(LIB_VERSION_APP)
	$(LN_S) -f $(LIB_STATIC).$(LIB_VERSION_APP) $(libdir)/$(LIB_STATIC)

install_pkgconfig:
	$(INSTALL) -D $(OWNER) $(GROUP) -m 444 $(LIB_NAME).pc \
		$(usrlibdir)/pkgconfig/lvm2app.pc

liblvm.cflow: $(SOURCES)
	set -e; (echo -n "SOURCES += "; \
		 echo $(SOURCES) | \
		 sed "s/^/ /;s/ / $(top_srcdir)\/liblvm\//g;s/$$//"; \
		 ) > $@

cflow: liblvm.cflow

CLEAN_TARGETS += $(LIB_NAME).$(LIB_SUFFIX)

distclean_lib:
	$(RM) $(LIB_NAME).pc

distclean: distclean_lib
