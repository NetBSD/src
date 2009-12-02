#!/bin/bash
# Copyright (C) 2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

. ./test-utils.sh

aux prepare_vg 3

lvcreate -m 1 -l 1 -n mirror $vg
lvcreate -l 1 -n resized $vg
lvchange -a n $vg/mirror

backup_dev $devs

init() {
	restore_dev $devs
	lvs -o lv_name,lv_size --units k $vg | tee lvs.out
	grep resized lvs.out | not grep 8192
	lvresize -L 8192K $vg/resized
	restore_dev $dev1
}

check() {
	lvs -o lv_name,lv_size --units k $vg | tee lvs.out
	grep resized lvs.out | grep 8192
}

# vgscan fixes up metadata
init
vgscan 2>&1 | tee cmd.out
grep "Inconsistent metadata found for VG $vg" cmd.out
vgscan 2>&1 | tee cmd.out
not grep "Inconsistent metadata found for VG $vg" cmd.out
check

# vgdisplay fixes
init
vgdisplay 2>&1 | tee cmd.out
grep "Inconsistent metadata found for VG $vg" cmd.out
vgdisplay 2>&1 | tee cmd.out
not grep "Inconsistent metadata found for VG $vg" cmd.out
check

# lvs fixes up
init
lvs 2>&1 | tee cmd.out
grep "Inconsistent metadata found for VG $vg" cmd.out
vgdisplay 2>&1 | tee cmd.out
not grep "Inconsistent metadata found for VG $vg" cmd.out
check

# vgs fixes up as well
init
vgs 2>&1 | tee cmd.out
grep "Inconsistent metadata found for VG $vg" cmd.out
vgs 2>&1 | tee cmd.out
not grep "Inconsistent metadata found for VG $vg" cmd.out
check
