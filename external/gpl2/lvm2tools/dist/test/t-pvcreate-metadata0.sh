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

#
# Testcase for bugzilla #450651
# also checks that vgremove properly removes all lv devices in the right order
#
# 'Test pvcreate without metadata on all pvs'

. ./test-utils.sh

aux prepare_devs 2 128

#lv_snap=$lv2
pvcreate "$dev1"
pvcreate --metadatacopies 0 "$dev2"

# "check lv snapshot" 
vgcreate -c n "$vg" "$dev1" "$dev2" 
lvcreate -n "$lv" -l 60%FREE "$vg" 
lvcreate -s -n $lv2 -l 10%FREE "$vg"/"$lv" 
pvdisplay 
lvdisplay
vgremove -f "$vg"
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

#
# Testcase for bugzilla #450651
# also checks that vgremove properly removes all lv devices in the right order
#
# 'Test pvcreate without metadata on all pvs'

. ./test-utils.sh

aux prepare_devs 2 128

#lv_snap=$lv2
pvcreate "$dev1"
pvcreate --metadatacopies 0 "$dev2"

# "check lv snapshot" 
vgcreate -c n "$vg" "$dev1" "$dev2" 
lvcreate -n "$lv" -l 60%FREE "$vg" 
lvcreate -s -n $lv2 -l 10%FREE "$vg"/"$lv" 
pvdisplay 
lvdisplay
vgremove -f "$vg"
