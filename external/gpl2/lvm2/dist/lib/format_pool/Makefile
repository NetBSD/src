#
# Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
# Copyright (C) 2004 Red Hat, Inc. All rights reserved.
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


SOURCES =\
	disk_rep.c \
	format_pool.c \
	import_export.c \
	pool_label.c

LIB_SHARED = liblvm2formatpool.so
LIB_VERSION = $(LIB_VERSION_LVM)

include ../../make.tmpl

install: liblvm2formatpool.so
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(STRIP) $< \
		$(libdir)/liblvm2formatpool.so.$(LIB_VERSION)
	$(LN_S) -f liblvm2formatpool.so.$(LIB_VERSION) \
		$(libdir)/liblvm2formatpool.so


