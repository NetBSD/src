#
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
top_srcdir = ..
top_builddir = ..


CONFSRC=example.conf
CONFDEST=lvm.conf

include ../make.tmpl

install_lvm2: $(CONFSRC)
	@if [ ! -e $(confdir)/$(CONFDEST) ]; then \
		echo "Installing $(CONFSRC) as $(confdir)/$(CONFDEST)"; \
		/usr/bin/install -c -D $(OWNER) $(GROUP) -m 644 $< \
			$(confdir)/$(CONFDEST); \
	fi

install: install_lvm2
