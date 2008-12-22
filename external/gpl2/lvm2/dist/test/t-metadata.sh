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

aux prepare_devs 5

pvcreate $dev1
pvcreate --metadatacopies 0 $dev2
pvcreate --metadatacopies 0 $dev3
pvcreate $dev4
pvcreate --metadatacopies 0 $dev5

vgcreate -c n "$vg" $devs
lvcreate -n $lv -l 1 -i5 -I256 $vg

pvchange -x n $dev1
pvchange -x y $dev1
vgchange -a n $vg
pvchange --uuid $dev1
pvchange --uuid $dev2
vgremove -f $vg

pvcreate -M1 $dev1
pvcreate -M1 $dev2
pvcreate -M1 $dev3
vgcreate -M1 $vg $dev1 $dev2 $dev3
pvchange --uuid $dev1
