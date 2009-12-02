#
# Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
# Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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


ifeq ("", "yes")
FSADMMAN = fsadm.8
else
FSADMMAN =
endif

MAN5=lvm.conf.5
MAN8=lvchange.8 lvconvert.8 lvcreate.8 lvdisplay.8 lvextend.8 lvm.8 \
	lvmchange.8 lvmdiskscan.8 lvmdump.8 \
	lvreduce.8 lvremove.8 lvrename.8 lvresize.8 lvs.8 \
	lvscan.8 pvchange.8 pvck.8 pvcreate.8 pvdisplay.8 pvmove.8 pvremove.8 \
	pvresize.8 pvs.8 pvscan.8 vgcfgbackup.8 vgcfgrestore.8 vgchange.8 \
	vgck.8 vgcreate.8 vgconvert.8 vgdisplay.8 vgexport.8 vgextend.8 \
	vgimport.8 vgimportclone.8 vgmerge.8 vgmknodes.8 vgreduce.8 vgremove.8 \
	vgrename.8 vgs.8 vgscan.8 vgsplit.8 $(FSADMMAN)
MAN8CLUSTER=clvmd.8
MAN8DM=dmsetup.8
MAN5DIR=${mandir}/man5
MAN8DIR=${mandir}/man8

CLEAN_TARGETS=$(MAN5) $(MAN8) $(MAN8CLUSTER) $(FSADMMAN) $(MAN8DM)

include ../make.tmpl

ifneq ("none", "none")
  install: install_cluster
endif

all: man

.PHONY: man

device-mapper: $(MAN8DM)

man: $(MAN5) $(MAN8) $(MAN8CLUSTER)

$(MAN5) $(MAN8) $(MAN8CLUSTER):	Makefile

%: %.in
	$(SED) -e "s/#VERSION#/$(LVM_VERSION)/" $< > $@

install_lvm2:
	@echo "Installing $(MAN8) in $(MAN8DIR)"
	@for f in $(MAN8); \
	do \
	   $(RM) $(MAN8DIR)/$$f; \
	   /usr/bin/install -c -D $(OWNER) $(GROUP) -m 444 $$f $(MAN8DIR)/$$f; \
	done

	@echo "Installing $(MAN5) in $(MAN5DIR)"
	@for f in $(MAN5); \
	do \
	   $(RM) $(MAN5DIR)/$$f; \
	   /usr/bin/install -c -D $(OWNER) $(GROUP) -m 444 $$f $(MAN5DIR)/$$f; \
	done

install_cluster:
	@echo "Installing $(MAN8CLUSTER) in $(MAN8DIR)"
	@for f in $(MAN8CLUSTER); \
	do \
	   $(RM) $(MAN8DIR)/$$f; \
	   /usr/bin/install -c -D $(OWNER) $(GROUP) -m 444 $$f $(MAN8DIR)/$$f; \
	done

install_device-mapper:
	@echo "Installing $(MAN8DM) in $(MAN8DIR)"
	@for f in $(MAN8DM); \
	do \
	   $(RM) $(MAN8DIR)/$$f; \
	   /usr/bin/install -c -D $(OWNER) $(GROUP) -m 444 $$f $(MAN8DIR)/$$f; \
	done

install: install_lvm2 install_device-mapper
