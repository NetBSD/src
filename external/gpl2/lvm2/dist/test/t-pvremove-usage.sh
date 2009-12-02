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

aux prepare_devs 3
pvcreate $dev1
pvcreate --metadatacopies 0 $dev2
pvcreate --metadatacopies 2 $dev3
pvremove $dev2

# failing, but still removing everything what can be removed
# is somewhat odd as default, what do we have -f for?
pvs | not grep $dev2
pvcreate  --metadatacopies 0 $dev2

# check pvremove refuses to remove pv in a vg
vgcreate -c n $vg $dev1 $dev2
not pvremove $dev2 $dev3

for mdacp in 0 1 2; do
    # check pvremove truly wipes the label (pvscan wont find) (---metadatacopies $mdacp)
    pvcreate --metadatacopies $mdacp $dev3
    pvremove $dev3
    # try to remove agail - should fail cleanly
    not pvremove $dev3
    pvscan | not grep $dev3

	# bz179473 refuse to wipe non-PV device without -f
    not pvremove $dev3
    pvremove -f $dev3

    # reset setup
    vgremove -ff $vg
    pvcreate --metadatacopies $mdacp $dev1
    pvcreate $dev2
    vgcreate $vg $dev1 $dev2 

    # pvremove -f fails when pv in a vg (---metadatacopies $mdacp)
    not pvremove -f $dev1
    pvs $dev1

    # pvremove -ff fails without confirmation when pv in a vg (---metadatacopies $mdacp)
    echo n | not pvremove -ff $dev1

    # pvremove -ff succeds with confirmation when pv in a vg (---metadatacopies $mdacp)
    yes | pvremove -ff $dev1
    not pvs $dev1

    vgreduce --removemissing $vg
    pvcreate --metadatacopies $mdacp $dev1
    vgextend $vg $dev1

    # pvremove -ff -y is sufficient when pv in a vg (---metadatacopies $mdacp)" '
    echo n | pvremove -ff -y $dev1

    vgreduce --removemissing $vg
    pvcreate --metadatacopies $mdacp $dev1
    vgextend $vg $dev1
done
