#
# Copyright (C) 2006 Red Hat, Inc. All rights reserved.
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


include ../make.tmpl

install:
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(srcdir)/lvm_dump.sh \
		$(sbindir)/lvmdump
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(srcdir)/vgimportclone.sh \
		$(sbindir)/vgimportclone
ifeq ("", "yes")
	$(INSTALL) -D $(OWNER) $(GROUP) -m 555 $(srcdir)/fsadm.sh \
		$(sbindir)/fsadm
endif

install_lvm2: install

DISTCLEAN_TARGETS += clvmd_init_red_hat lvm2_monitoring_init_red_hat
