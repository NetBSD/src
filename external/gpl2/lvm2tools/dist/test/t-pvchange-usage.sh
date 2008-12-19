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

# 'Test pvchange option values'

. ./test-utils.sh

aux prepare_devs 4

for mda in 0 1 2 
do
# "setup pv with metadatacopies = $mda" 
	pvcreate $dev4 
	pvcreate --metadatacopies $mda $dev1 
	vgcreate $vg1 $dev1 $dev4 

# "pvchange adds/dels tag to pvs with metadatacopies = $mda " 
	pvchange $dev1 --addtag test$mda 
	check_pv_field_ $dev1 pv_tags test$mda 
	pvchange $dev1 --deltag test$mda 
	check_pv_field_ $dev1 pv_tags " "

# "vgchange disable/enable allocation for pvs with metadatacopies = $mda (bz452982)"
	pvchange $dev1 -x n 
	check_pv_field_ $dev1 pv_attr  --  
	pvchange $dev1 -x y 
	check_pv_field_ $dev1 pv_attr  a- 

# 'remove pv'
	vgremove $vg1 
	pvremove $dev1 $dev4
done

# "pvchange uuid"
pvcreate --metadatacopies 0 $dev1 
pvcreate --metadatacopies 2 $dev2 
vgcreate $vg1 $dev1 $dev2 
pvchange -u $dev1 
pvchange -u $dev2 
vg_validate_pvlv_counts_ $vg1 2 0 0

# "pvchange rejects uuid change under an active lv" 
lvcreate -l 16 -i 2 -n $lv --alloc anywhere $vg1 
vg_validate_pvlv_counts_ $vg1 2 1 0 
not pvchange -u $dev1
lvchange -an "$vg1"/"$lv" 
pvchange -u $dev1

# "cleanup" 
lvremove -f "$vg1"/"$lv"
vgremove $vg1

# "pvchange reject --addtag to lvm1 pv"
pvcreate -M1 $dev1 
not pvchange $dev1 --addtag test

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

# 'Test pvchange option values'

. ./test-utils.sh

aux prepare_devs 4

for mda in 0 1 2 
do
# "setup pv with metadatacopies = $mda" 
	pvcreate $dev4 
	pvcreate --metadatacopies $mda $dev1 
	vgcreate $vg1 $dev1 $dev4 

# "pvchange adds/dels tag to pvs with metadatacopies = $mda " 
	pvchange $dev1 --addtag test$mda 
	check_pv_field_ $dev1 pv_tags test$mda 
	pvchange $dev1 --deltag test$mda 
	check_pv_field_ $dev1 pv_tags " "

# "vgchange disable/enable allocation for pvs with metadatacopies = $mda (bz452982)"
	pvchange $dev1 -x n 
	check_pv_field_ $dev1 pv_attr  --  
	pvchange $dev1 -x y 
	check_pv_field_ $dev1 pv_attr  a- 

# 'remove pv'
	vgremove $vg1 
	pvremove $dev1 $dev4
done

# "pvchange uuid"
pvcreate --metadatacopies 0 $dev1 
pvcreate --metadatacopies 2 $dev2 
vgcreate $vg1 $dev1 $dev2 
pvchange -u $dev1 
pvchange -u $dev2 
vg_validate_pvlv_counts_ $vg1 2 0 0

# "pvchange rejects uuid change under an active lv" 
lvcreate -l 16 -i 2 -n $lv --alloc anywhere $vg1 
vg_validate_pvlv_counts_ $vg1 2 1 0 
not pvchange -u $dev1
lvchange -an "$vg1"/"$lv" 
pvchange -u $dev1

# "cleanup" 
lvremove -f "$vg1"/"$lv"
vgremove $vg1

# "pvchange reject --addtag to lvm1 pv"
pvcreate -M1 $dev1 
not pvchange $dev1 --addtag test

