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

# 'Check extents percentage arguments'

. ./test-utils.sh

aux prepare_vg 2 128

lvcreate -L 64m -n $lv $vg

# 'lvextend rejects both size and extents without PVs'
not lvextend -l 10 -L 64m $vg/$lv 2>err
grep "^  Please specify either size or extents but not both.\$" err

# 'lvextend rejects both size and extents with PVs'
not lvextend -l 10 -L 64m $vg/$lv $dev1 2>err
grep "^  Please specify either size or extents but not both.\$" err

# 'lvextend accepts no size or extents but one PV - bz154691'
lvextend $vg/$lv $dev1 >out
grep "^  Logical volume $lv successfully resized\$" out
check_pv_field_ $dev1 pv_free "0"

lvremove -f $vg/$lv 

# 'lvextend computes necessary free space correctly - bz213552'
vgsize=$(vgs -o vg_extent_count --noheadings)
lvcreate -l $vgsize  -n $lv $vg
yes | lvreduce -l $(( $vgsize / 2 )) $vg/$lv
lvextend -l $vgsize $vg/$lv

# 'Reset LV to original size' 
lvremove -f $vg/$lv 
lvcreate -L 64m -n $lv $vg

# 'lvextend accepts no size but extents 100%PVS and two PVs - bz154691'
lvextend -l +100%PVS $vg/$lv $dev1 $dev2 >out
grep "^  Logical volume $lv successfully resized\$" out 
check_pv_field_ $dev1 pv_free "0" 
check_pv_field_ $dev2 pv_free "0"

# Exercise the range overlap code.  Allocate every 2 extents.
#
#      Physical Extents
#          1         2
#012345678901234567890123
#
#aaXXaaXXaaXXaaXXaaXXaaXX - (a)llocated
#rrrXXXrrrXXXrrrXXXrrrXXX - (r)ange on cmdline
#ooXXXXXXoXXXooXXXXXXoXXX - (o)verlap of range and allocated
#
# Key: a - allocated
#      F - free
#      r - part of a range on the cmdline
#      N - not on cmdline
#
# Create the LV with 12 extents, allocated every other 2 extents.
# Then extend it, with a range of PVs on the cmdline of every other 3 extents.
# Total number of extents should be 12 + overlap = 12 + 6 = 18.
# Thus, total size for the LV should be 18 * 4M = 72M
#
# 'Reset LV to 12 extents, allocate every other 2 extents' 
create_pvs=`for i in $(seq 0 4 20); do echo -n "\$dev1:$i-$(($i + 1)) "; done` 
lvremove -f $vg/$lv
lvcreate -l 12 -n $lv $vg $create_pvs
check_lv_field_ $vg/$lv lv_size "48.00m"

# 'lvextend with partially allocated PVs and extents 100%PVS with PE ranges' 
extend_pvs=`for i in $(seq 0 6 18); do echo -n "\$dev1:$i-$(($i + 2)) "; done` 
lvextend -l +100%PVS $vg/$lv $extend_pvs >out
grep "^  Logical volume $lv successfully resized\$" out 
check_lv_field_ $vg/$lv lv_size "72.00m"

# Simple seg_count validation; initially create the LV with half the # of
# extents (should be 1 lv segment), extend it (should go to 2 segments),
# then reduce (should be back to 1)
# FIXME: test other segment fields such as seg_size, pvseg_start, pvseg_size
lvremove -f $vg/$lv
pe_count=$(pvs -o pv_pe_count --noheadings $dev1)
pe1=$(( $pe_count / 2 ))
lvcreate -l $pe1 -n $lv $vg
pesize=$(lvs -ovg_extent_size --units b --nosuffix --noheadings $vg/$lv)
segsize=$(( $pe1 * $pesize / 1024 / 1024 ))m
check_lv_field_ $vg/$lv seg_count 1
check_lv_field_ $vg/$lv seg_start 0
check_lv_field_ $vg/$lv seg_start_pe 0
#check_lv_field_ $vg/$lv seg_size $segsize
lvextend -l +$(( $pe_count * 1 )) $vg/$lv
check_lv_field_ $vg/$lv seg_count 2
lvreduce -f -l -$(( $pe_count * 1 )) $vg/$lv
check_lv_field_ $vg/$lv seg_count 1

