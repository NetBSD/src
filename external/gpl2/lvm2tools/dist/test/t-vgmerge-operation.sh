#!/bin/sh
# Copyright (C) 2007-2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

test_description='Test vgmerge operation'

. ./test-utils.sh

aux prepare_pvs 4 64

# 'vgmerge succeeds with single linear LV in source VG'
vgcreate $vg1 $dev1 $dev2 
vgcreate $vg2 $dev3 $dev4 
lvcreate -l 4 -n $lv1 $vg1 $dev1 
vgchange -an $vg1 
vg_validate_pvlv_counts_ $vg1 2 1 0 
vg_validate_pvlv_counts_ $vg2 2 0 0 
vgmerge $vg2 $vg1 
vg_validate_pvlv_counts_ $vg2 4 1 0 
vgremove -f $vg2

# 'vgmerge succeeds with single linear LV in source and destination VG'
vgcreate $vg1 $dev1 $dev2 
vgcreate $vg2 $dev3 $dev4 
lvcreate -l 4 -n $lv1 $vg1 
lvcreate -l 4 -n $lv2 $vg2 
vgchange -an $vg1 
vgchange -an $vg2 
vg_validate_pvlv_counts_ $vg1 2 1 0 
vg_validate_pvlv_counts_ $vg2 2 1 0 
vgmerge $vg2 $vg1 
vg_validate_pvlv_counts_ $vg2 4 2 0 
vgremove -f $vg2

# 'vgmerge succeeds with linear LV + snapshots in source VG'
vgcreate $vg1 $dev1 $dev2 
vgcreate $vg2 $dev3 $dev4 
lvcreate -l 16 -n $lv1 $vg1 
lvcreate -l 4 -s -n $lv2 $vg1/$lv1 
vgchange -an $vg1 
vg_validate_pvlv_counts_ $vg1 2 2 1 
vg_validate_pvlv_counts_ $vg2 2 0 0 
vgmerge $vg2 $vg1 
vg_validate_pvlv_counts_ $vg2 4 2 1 
lvremove -f $vg2/$lv2 
vgremove -f $vg2

# 'vgmerge succeeds with mirrored LV in source VG'
vgcreate $vg1 $dev1 $dev2 $dev3 
vgcreate $vg2 $dev4 
lvcreate -l 4 -n $lv1 -m1 $vg1 
vgchange -an $vg1 
vg_validate_pvlv_counts_ $vg1 3 1 0 
vg_validate_pvlv_counts_ $vg2 1 0 0 
vgmerge $vg2 $vg1 
vg_validate_pvlv_counts_ $vg2 4 1 0 
lvremove -f $vg2/$lv1 
vgremove -f $vg2

# 'vgmerge rejects LV name collision'
vgcreate $vg1 $dev1 $dev2
vgcreate $vg2 $dev3 $dev4
lvcreate -l 4 -n $lv1 $vg1
lvcreate -l 4 -n $lv1 $vg2
vgchange -an $vg1
aux vg_validate_pvlv_counts_ $vg1 2 1 0
aux vg_validate_pvlv_counts_ $vg2 2 1 0
not vgmerge $vg2 $vg1 2>err
grep "Duplicate logical volume name \"$lv1\" in \"$vg2\" and \"$vg1" err
aux vg_validate_pvlv_counts_ $vg1 2 1 0
aux vg_validate_pvlv_counts_ $vg2 2 1 0
vgremove -f $vg1
vgremove -f $vg2

