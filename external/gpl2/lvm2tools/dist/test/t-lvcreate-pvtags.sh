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

aux prepare_pvs 3
# not required, just testing
aux pvcreate --metadatacopies 0 $dev1

vgcreate $vg $devs
pvchange --addtag fast $devs

# 3 stripes with 3 PVs (selected by tag, @fast) is fine
lvcreate -l3 -i3 $vg @fast

# too many stripes(4) for 3 PVs
not lvcreate -l4 -i4 $vg @fast

# 2 stripes is too many with just one PV
not lvcreate -l2 -i2 $vg $G_dev_/mapper/pv1

# lvcreate mirror
lvcreate -l1 -m1 $vg @fast

# lvcreate mirror w/corelog
lvcreate -l1 -m2 --corelog $vg @fast

# lvcreate mirror w/no free PVs
not lvcreate -l1 -m2 $vg @fast

# lvcreate mirror (corelog, w/no free PVs)
not lvcreate -l1 -m3 --corelog $vg @fast

# lvcreate mirror with a single PV arg
not lvcreate -l1 -m1 --corelog $vg $dev1
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

aux prepare_pvs 3
# not required, just testing
aux pvcreate --metadatacopies 0 $dev1

vgcreate $vg $devs
pvchange --addtag fast $devs

# 3 stripes with 3 PVs (selected by tag, @fast) is fine
lvcreate -l3 -i3 $vg @fast

# too many stripes(4) for 3 PVs
not lvcreate -l4 -i4 $vg @fast

# 2 stripes is too many with just one PV
not lvcreate -l2 -i2 $vg $G_dev_/mapper/pv1

# lvcreate mirror
lvcreate -l1 -m1 $vg @fast

# lvcreate mirror w/corelog
lvcreate -l1 -m2 --corelog $vg @fast

# lvcreate mirror w/no free PVs
not lvcreate -l1 -m2 $vg @fast

# lvcreate mirror (corelog, w/no free PVs)
not lvcreate -l1 -m3 --corelog $vg @fast

# lvcreate mirror with a single PV arg
not lvcreate -l1 -m1 --corelog $vg $dev1
