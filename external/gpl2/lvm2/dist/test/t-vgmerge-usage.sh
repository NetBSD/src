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

# 'Test vgmerge command options for validity'

. ./test-utils.sh

aux prepare_pvs 4

# 'vgmerge normal operation'
# ensure ordering does not matter
vgcreate $vg1 $dev1 $dev2 
vgcreate $vg2 $dev3 $dev4 
vgmerge $vg1 $vg2 
vgremove $vg1
vgcreate $vg2 $dev1 $dev2
vgcreate $vg1 $dev3 $dev4
vgmerge $vg2 $vg1
vgremove $vg2

# 'vgmerge rejects duplicate vg name'
vgcreate $vg1 $dev1 $dev2 
vgcreate $vg2 $dev3 $dev4 
not vgmerge $vg1 $vg1 2>err
grep "^  Duplicate volume group name \"$vg1\"\$" err 
vgremove $vg2 
vgremove $vg1

# 'vgmerge rejects vgs with incompatible extent_size'
vgcreate --physicalextentsize 4M $vg1 $dev1 $dev2 
vgcreate --physicalextentsize 8M $vg2 $dev3 $dev4 
not vgmerge $vg1 $vg2 2>err
grep "^  Extent sizes differ" err 
vgremove $vg2 
vgremove $vg1

# 'vgmerge rejects vgmerge because max_pv is exceeded'
vgcreate --maxphysicalvolumes 2 $vg1 $dev1 $dev2 
vgcreate --maxphysicalvolumes 2 $vg2 $dev3 $dev4 
not vgmerge $vg1 $vg2 2>err
grep "^  Maximum number of physical volumes (2) exceeded" err 
vgremove $vg2 
vgremove $vg1

# 'vgmerge rejects vg with active lv'
vgcreate $vg1 $dev1 $dev2 
vgcreate $vg2 $dev3 $dev4 
lvcreate -l 4 -n lv1 $vg2 
not vgmerge $vg1 $vg2 2>err
grep "^  Logical volumes in \"$vg2\" must be inactive\$" err 
vgremove -f $vg2 
vgremove -f $vg1

# 'vgmerge rejects vgmerge because max_lv is exceeded' 
vgcreate --maxlogicalvolumes 2 $vg1 $dev1 $dev2 
vgcreate --maxlogicalvolumes 2 $vg2 $dev3 $dev4 
lvcreate -l 4 -n lv1 $vg1 
lvcreate -l 4 -n lv2 $vg1 
lvcreate -l 4 -n lv3 $vg2 
vgchange -an $vg1 
vgchange -an $vg2 
not vgmerge $vg1 $vg2 2>err
grep "^  Maximum number of logical volumes (2) exceeded" err 
vgremove -f $vg2 
vgremove -f $vg1

