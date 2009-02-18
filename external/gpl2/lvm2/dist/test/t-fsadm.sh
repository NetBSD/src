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

exit 200
# doesn't work right now
aux prepare_pvs 2
aux pvcreate --metadatacopies 0 $dev1
vgcreate -cn $vg $devs

# Test for block sizes != 1024 (rhbz #480022)
lvcreate -n$lv1 -L 64M $vg
mke2fs -b4096 -j $G_dev_/$vg/$lv1
e2fsck -f -y $G_dev_/$vg/$lv1
fsadm --lvresize resize $G_dev_/$vg/$lv1 128M
vgremove -ff $vg
