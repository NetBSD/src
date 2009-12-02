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

test_description='Exercise some vgcreate diagnostics'

. ./test-utils.sh

aux prepare_devs 3
pvcreate $dev1 $dev2
pvcreate --metadatacopies 0 $dev3

#COMM 'vgcreate accepts 8.00m physicalextentsize for VG'
vgcreate $vg --physicalextentsize 8.00m $dev1 $dev2
check_vg_field_ $vg vg_extent_size 8.00m
vgremove $vg
# try vgck and to remove it again - should fail (but not segfault)
not vgremove $vg
not vgck $vg

#COMM 'vgcreate accepts smaller (128) maxlogicalvolumes for VG'
vgcreate $vg --maxlogicalvolumes 128 $dev1 $dev2 
check_vg_field_ $vg max_lv 128 
vgremove $vg

#COMM 'vgcreate accepts smaller (128) maxphysicalvolumes for VG'
vgcreate $vg --maxphysicalvolumes 128 $dev1 $dev2
check_vg_field_ $vg max_pv 128
vgremove $vg

#COMM 'vgcreate rejects a zero physical extent size'
not vgcreate --physicalextentsize 0 $vg $dev1 $dev2 2>err
grep "^  Physical extent size may not be zero\$" err

#COMM 'vgcreate rejects "inherit" allocation policy'
not vgcreate --alloc inherit $vg $dev1 $dev2 2>err
grep "^  Volume Group allocation policy cannot inherit from anything\$" err

#COMM 'vgcreate rejects vgname "."'
vginvalid=.; 
not vgcreate $vginvalid $dev1 $dev2 2>err
grep "New volume group name \"$vginvalid\" is invalid\$" err

#COMM 'vgcreate rejects vgname greater than 128 characters'
vginvalid=thisnameisridiculouslylongtotestvalidationcodecheckingmaximumsizethisiswhathappenswhenprogrammersgetboredandorarenotcreativedonttrythisathome
not vgcreate $vginvalid $dev1 $dev2 2>err
grep "New volume group name \"$vginvalid\" is invalid\$" err

#COMM 'vgcreate rejects already existing vgname "/tmp/$vg"'
#touch /tmp/$vg
#not vgcreate $vg $dev1 $dev2 2>err
#grep "New volume group name \"$vg\" is invalid\$" err

#COMM "vgcreate rejects repeated invocation (run 2 times) (bz178216)"
vgcreate $vg $dev1 $dev2
not vgcreate $vg $dev1 $dev2
vgremove -ff $vg

#COMM 'vgcreate rejects MaxLogicalVolumes > 255'
not vgcreate --metadatatype 1 --maxlogicalvolumes 1024 $vg $dev1 $dev2 2>err
grep "^  Number of volumes may not exceed 255\$" err

#COMM "vgcreate fails when the only pv has --metadatacopies 0"
not vgcreate $vg $dev3

# Test default (4MB) vg_extent_size as well as limits of extent_size
not vgcreate --physicalextentsize 0k $vg $dev1 $dev2
vgcreate --physicalextentsize 1k $vg $dev1 $dev2
check_vg_field_ $vg vg_extent_size 1.00k
vgremove -ff $vg
not vgcreate --physicalextentsize 3K $vg $dev1 $dev2
not vgcreate --physicalextentsize 1024t $vg $dev1 $dev2
#not vgcreate --physicalextentsize 1T $vg $dev1 $dev2
# FIXME: vgcreate allows physicalextentsize larger than pv size!

# Test default max_lv, max_pv, extent_size, alloc_policy, clustered
vgcreate $vg $dev1 $dev2
check_vg_field_ $vg vg_extent_size 4.00m
check_vg_field_ $vg max_lv 0
check_vg_field_ $vg max_pv 0
check_vg_field_ $vg vg_attr "wz--n-"
vgremove -ff $vg

# Implicit pvcreate tests, test pvcreate options on vgcreate
# --force, --yes, --metadata{size|copies|type}, --zero
# --dataalignment[offset]
pvremove $dev1 $dev2
vgcreate --force --yes --zero y $vg $dev1 $dev2
vgremove -f $vg
pvremove -f $dev1

for i in 0 1 2 3
do
# vgcreate (lvm2) succeeds writing LVM label at sector $i
    vgcreate --labelsector $i $vg $dev1
    dd if=$dev1 bs=512 skip=$i count=1 2>/dev/null | strings | grep -q LABELONE;
    vgremove -f $vg
    pvremove -f $dev1
done

# pvmetadatacopies
for i in 1 2
do
    vgcreate --pvmetadatacopies $i $vg $dev1
    check_pv_field_ $dev1 pv_mda_count $i
    vgremove -f $vg
    pvremove -f $dev1
done
not vgcreate --metadatacopies 0 $vg $dev1
pvcreate --metadatacopies 1 $dev2
vgcreate --pvmetadatacopies 0 $vg $dev1 $dev2
check_pv_field_ $dev1 pv_mda_count 0
check_pv_field_ $dev2 pv_mda_count 1
vgremove -f $vg
pvremove -f $dev1

# metadatasize, dataalignment, dataalignmentoffset
#COMM 'pvcreate sets data offset next to mda area'
vgcreate --metadatasize 100k --dataalignment 100k $vg $dev1
check_pv_field_ $dev1 pe_start 200.00k
vgremove -f $vg
pvremove -f $dev1

# data area is aligned to 64k by default,
# data area start is shifted by the specified alignment_offset
pv_align="195.50k"
vgcreate --metadatasize 128k --dataalignmentoffset 7s $vg $dev1
check_pv_field_ $dev1 pe_start $pv_align
vgremove -f $vg
pvremove -f $dev1

# metadatatype
for i in 1 2
do
    vgcreate -M $i $vg $dev1
    check_vg_field_ $vg vg_fmt lvm$i
    vgremove -f $vg
    pvremove -f $dev1
done

# vgcreate fails if pv belongs to existing vg
vgcreate $vg1 $dev1 $dev2
not vgcreate $vg2 $dev2
vgremove -f $vg1
pvremove -f $dev1 $dev2

# all PVs exist in the VG after created
pvcreate $dev1
vgcreate $vg1 $dev1 $dev2 $dev3
check_pv_field_ $dev1 vg_name $vg1
check_pv_field_ $dev2 vg_name $vg1
check_pv_field_ $dev3 vg_name $vg1
vgremove -f $vg1
pvremove -f $dev1 $dev2 $dev3
