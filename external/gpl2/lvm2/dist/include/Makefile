#
# Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
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

SHELL = /bin/sh

srcdir = .
top_srcdir = ..
top_builddir = ..


LN_S = ln -s

.PHONY: clean distclean all install pofile install_cluster install_device-mapper

all: .symlinks_created

.symlinks_created: .symlinks 
	find . -maxdepth 1 -type l -exec $(RM) \{\} \;
	for i in `cat $<`; do $(LN_S) $$i ; done
	touch $@

distclean:
	find . -maxdepth 1 -type l -exec $(RM) \{\} \;
	$(RM) Makefile .include_symlinks .symlinks_created .symlinks

pofile: all

device-mapper: all

clean:

install:

install_cluster:

install_device-mapper:

install_lvm2:

cflow:

