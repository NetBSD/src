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

. ./test-utils.sh

aux prepare_pvs 4

# vgcfgbackup handles similar VG names (bz458941)
vg1=${PREFIX}vg00
vg2=${PREFIX}vg01
vgcreate $vg1 $dev1
vgcreate $vg2 $dev2
vgcfgbackup -f /tmp/bak-%s >out
grep "Volume group \"$vg1\" successfully backed up." out
grep "Volume group \"$vg2\" successfully backed up." out
vgremove -ff $vg1
vgremove -ff $vg2

# vgcfgbackup correctly stores metadata with missing PVs
# and vgcfgrestore able to restore them when device reappears
pv1_uuid=$(pvs --noheadings -o pv_uuid $dev1)
pv2_uuid=$(pvs --noheadings -o pv_uuid $dev2)
vgcreate $vg $devs
lvcreate -l1 -n $lv1 $vg $dev1
lvcreate -l1 -n $lv2 $vg $dev2
lvcreate -l1 -n $lv3 $vg $dev3
vgchange -a n $vg
pvcreate -ff -y $dev1
pvcreate -ff -y $dev2
vgcfgbackup -f "$(pwd)/backup.$$" $vg
sed 's/flags = \[\"MISSING\"\]/flags = \[\]/' "$(pwd)/backup.$$" > "$(pwd)/backup.$$1"
pvcreate -ff -y -u $pv1_uuid $dev1
pvcreate -ff -y -u $pv2_uuid $dev2
vgcfgrestore -f "$(pwd)/backup.$$1" $vg
