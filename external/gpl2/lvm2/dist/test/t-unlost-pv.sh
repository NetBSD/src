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

aux prepare_vg 3

lvcreate -m 1 -l 1 -n mirror $vg
lvchange -a n $vg/mirror

check() {
vgscan 2>&1 | tee vgscan.out
grep "Inconsistent metadata found for VG $vg" vgscan.out
vgscan 2>&1 | tee vgscan.out
not grep "Inconsistent metadata found for VG $vg" vgscan.out
}

# try orphaning a missing PV (bz45867)
disable_dev $dev1
vgreduce --removemissing --force $vg
enable_dev $dev1
check

# try to just change metadata; we expect the new version (with MISSING_PV set
# on the reappeared volume) to be written out to the previously missing PV
vgextend $vg $dev1
disable_dev $dev1
lvremove $vg/mirror
enable_dev $dev1
check
