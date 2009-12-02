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

aux prepare_devs 4

for mdatype in 1 2
do
# pvcreate (lvm$mdatype) refuses to overwrite an mounted filesystem (bz168330)
	test ! -d $G_root_/mnt && mkdir $G_root_/mnt 
	if mke2fs $dev1; then
		mount $dev1 $G_root_/mnt
		not pvcreate -M$mdatype $dev1 2>err
		grep "Can't open $dev1 exclusively.  Mounted filesystem?" err
		umount $dev1
	fi

# pvcreate (lvm$mdatype) succeeds when run repeatedly (pv not in a vg) (bz178216)
    pvcreate -M$mdatype $dev1
    pvcreate -M$mdatype $dev1
    pvremove -f $dev1

# pvcreate (lvm$mdatype) fails when PV belongs to VG" \
    pvcreate -M$mdatype $dev1
    vgcreate -M$mdatype $vg1 $dev1
    not pvcreate -M$mdatype $dev1

    vgremove -f $vg1
    pvremove -f $dev1

# pvcreate (lvm$mdatype) fails when PV1 does and PV2 does not belong to VG
    pvcreate -M$mdatype $dev1
    pvcreate -M$mdatype $dev2
    vgcreate -M$mdatype $vg1 $dev1

# pvcreate a second time on $dev2 and $dev1
    not pvcreate -M$mdatype $dev2 $dev1

    vgremove -f $vg1
    pvremove -f $dev2
    pvremove -f $dev1

# NOTE: Force pvcreate after test completion to ensure clean device
#test_expect_success \
#  "pvcreate (lvm$mdatype) fails on md component device" \
#  'mdadm -C -l raid0 -n 2 /dev/md0 $dev1 $dev2 &&
#   pvcreate -M$mdatype $dev1;
#   status=$?; echo status=$status; test $status != 0 &&
#   mdadm --stop /dev/md0 &&
#   pvcreate -ff -y -M$mdatype $dev1 $dev2 &&
#   pvremove -f $dev1 $dev2'
done

# pvcreate (lvm2) fails without -ff when PV with metadatacopies=0 belongs to VG
pvcreate --metadatacopies 0 $dev1
pvcreate --metadatacopies 1 $dev2
vgcreate $vg1 $dev1 $dev2
not pvcreate $dev1
vgremove -f $vg1
pvremove -f $dev2
pvremove -f $dev1

# pvcreate (lvm2) succeeds with -ff when PV with metadatacopies=0 belongs to VG
pvcreate --metadatacopies 0 $dev1 
pvcreate --metadatacopies 1 $dev2
vgcreate $vg1 $dev1 $dev2 
pvcreate -ff -y $dev1 
vgreduce --removemissing $vg1 
vgremove -ff $vg1 
pvremove -f $dev2 
pvremove -f $dev1

for i in 0 1 2 3
do
# pvcreate (lvm2) succeeds writing LVM label at sector $i
    pvcreate --labelsector $i $dev1
    dd if=$dev1 bs=512 skip=$i count=1 2>/dev/null | strings | grep -q LABELONE;
    pvremove -f $dev1
done

# pvcreate (lvm2) fails writing LVM label at sector 4
not pvcreate --labelsector 4 $dev1

backupfile=mybackupfile-$(this_test_)
uuid1=freddy-fred-fred-fred-fred-fred-freddy
uuid2=freddy-fred-fred-fred-fred-fred-fredie
bogusuuid=fred

# pvcreate rejects uuid option with less than 32 characters
not pvcreate --uuid $bogusuuid $dev1

# pvcreate rejects uuid already in use
pvcreate --uuid $uuid1 $dev1
not pvcreate --uuid $uuid1 $dev2

# pvcreate rejects non-existent file given with restorefile
not pvcreate --uuid $uuid1 --restorefile $backupfile $dev1

# pvcreate rejects restorefile with uuid not found in file
pvcreate --uuid $uuid1 $dev1
vgcfgbackup -f $backupfile
not pvcreate --uuid $uuid2 --restorefile $backupfile $dev2

# pvcreate wipes swap signature when forced
dd if=/dev/zero of=$dev1 bs=1024 count=64
mkswap $dev1
blkid -c /dev/null $dev1 | grep "swap"
pvcreate -f $dev1
blkid -c /dev/null $dev1 | not grep "swap"
