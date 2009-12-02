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
top_srcdir = ..
top_builddir = ..


DM_RULES=10-dm.rules 13-dm-disk.rules 95-dm-notify.rules
LVM_RULES=11-dm-lvm.rules
DM_DIR=$(shell grep "\#define DM_DIR" $(top_srcdir)/libdm/misc/dm-ioctl.h | awk '{print $$3}')

CLEAN_TARGETS=10-dm.rules

include ../make.tmpl

%: %.in
	$(SED) -e "s/(DM_DIR)/$(DM_DIR)/" $< >$@

install_lvm2: $(LVM_RULES)
	@echo "Installing $(LVM_RULES) in $(udevdir)"
	@for f in $(LVM_RULES); \
	do \
	   $(RM) $(udevdir)/$$f; \
	   $(INSTALL) -c -D $(OWNER) $(GROUP) -m 644 $(srcdir)/$$f $(udevdir)/$$f; \
	done

install_device-mapper: $(DM_RULES)
	@echo "Installing $(DM_RULES) in $(udevdir)"
	@for f in $(DM_RULES); \
	do \
	   $(RM) $(udevdir)/$$f; \
	   $(INSTALL) -c -D $(OWNER) $(GROUP) -m 644 $(srcdir)/$$f $(udevdir)/$$f; \
	done

install: install_lvm2 install_device-mapper
