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

test_description='Exercise some vgchange diagnostics'

. ./test-utils.sh

aux prepare_pvs 3
pvcreate --metadatacopies 0 $dev1
vgcreate $vg $devs

get_vg_field() {
	local vg=$1
	local field=$2
	local value
	vgs --noheading -o $field $vg | sed 's/^ *//'
}

vgdisplay $vg

# vgchange -p MaxPhysicalVolumes (bz202232)
aux check_vg_field_ $vg max_pv 0
vgchange -p 128 $vg
aux check_vg_field_ $vg max_pv 128

pv_count=$(get_vg_field $vg pv_count)
not vgchange -p 2 $vg 2>err
grep "MaxPhysicalVolumes is less than the current number $pv_count of PVs for" err
aux check_vg_field_ $vg max_pv 128

# vgchange -l MaxLogicalVolumes
aux check_vg_field_ $vg max_lv 0
vgchange -l 128 $vg
aux check_vg_field_ $vg max_lv 128

lvcreate -l4 -n$lv1 $vg
lvcreate -l4 -n$lv2 $vg

lv_count=$(get_vg_field $vg lv_count)
not vgchange -l 1 $vg 2>err
grep "MaxLogicalVolume is less than the current number $lv_count of LVs for"  err
aux check_vg_field_ $vg max_lv 128

