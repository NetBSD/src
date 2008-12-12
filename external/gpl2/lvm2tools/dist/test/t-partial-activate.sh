. ./test-utils.sh

aux prepare_vg 3

lvcreate -m 1 -l 1 -n mirror $vg
lvchange -a n $vg/mirror
disable_dev $dev1

# FIXME should this return an error code due to that big fat WARNING?
vgreduce --removemissing $vg
not lvchange -v -a y $vg/mirror
lvchange -v --partial -a y $vg/mirror
