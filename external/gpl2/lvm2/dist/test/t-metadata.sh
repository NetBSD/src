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

# check that PVs without metadata don't cause too many full device rescans (bz452606)
for mdacp in 1 0; do
	pvcreate --metadatacopies $mdacp $devs
	pvcreate $dev1
	vgcreate $vg $devs
	lvcreate -n $lv1 -l 2 -i5 -I256 $vg
	lvcreate -n $lv2 -m2 -l 2  $vg
	#lvchange -an $vg
	lvchange -an $vg/$lv1 >out$mdacp 2>&1 
	lvchange -an $vg/$lv2 >>out$mdacp 2>&1 
	test ! -s out$mdacp
	vgchange -ay $vg
	lvchange -vvvv -an $vg/$lv1 >out$mdacp 2>&1 
	lvchange -vvvv -an $vg/$lv2 >>out$mdacp 2>&1 
	eval run$mdacp=$(wc -l <out$mdacp)
	vgremove -f $vg
done
not grep "Cached VG .* incorrect PV list" out0

# some M1 metadata tests
pvcreate -M1 $dev1
pvcreate -M1 $dev2
pvcreate -M1 $dev3
vgcreate -M1 $vg $dev1 $dev2 $dev3
pvchange --uuid $dev1
