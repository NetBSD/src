. ./test-utils.sh

prepare_lvmconf '[ "a/dev\/mirror/", "a/dev\/mapper\/.*$/", "r/.*/" ]'
aux prepare_devs 3

pvcreate $devs
vgcreate $vg1 $dev1 $dev2
lvcreate -n $lv1 -l 100%FREE $vg1

#top VG
pvcreate $G_dev_/$vg1/$lv1
vgcreate $vg $G_dev_/$vg1/$lv1 $dev3

vgchange -a n $vg
vgchange -a n $vg1

# this should fail but not segfault, RHBZ 481793.
not vgsplit $vg $vg1 $dev3
