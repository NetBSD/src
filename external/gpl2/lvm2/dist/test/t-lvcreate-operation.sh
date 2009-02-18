#!/bin/sh
# Copyright (C) 2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# 'Exercise some lvcreate diagnostics'

. ./test-utils.sh

cleanup_lvs() {
	lvremove -ff $vg
	if dmsetup table|grep $vg; then
		echo "ERROR: lvremove did leave some some mappings in DM behind!"
		return 1
	fi
}

aux prepare_pvs 2
aux pvcreate --metadatacopies 0 $dev1
aux vgcreate $vg $devs

#COMM create snapshots of LVs on --metadatacopies 0 PV (bz450651)
lvcreate -n$lv1 -l4 $vg $dev1
lvcreate -n$lv2 -l4 -s $vg/$lv1
#lvremove -f $vg/$lv2
cleanup_lvs

