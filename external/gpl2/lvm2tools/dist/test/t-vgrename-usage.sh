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
pvcreate $dev1 $dev2
pvcreate --metadatacopies 0 $dev3 $dev4

# vgrename normal operation - rename vg1 to vg2
vgcreate $vg1 $dev1 $dev2
vgrename $vg1 $vg2
check_vg_field_ $vg2 vg_name $vg2
vgremove $vg2

# vgrename by uuid (bz231187)
vgcreate $vg1 $dev1 $dev3
UUID=$(vgs --noheading -o vg_uuid $vg1)
check_vg_field_ $vg1 vg_uuid $UUID
vgrename $UUID $vg2
check_vg_field_ $vg2 vg_name $vg2
vgremove $vg2
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
pvcreate $dev1 $dev2
pvcreate --metadatacopies 0 $dev3 $dev4

# vgrename normal operation - rename vg1 to vg2
vgcreate $vg1 $dev1 $dev2
vgrename $vg1 $vg2
check_vg_field_ $vg2 vg_name $vg2
vgremove $vg2

# vgrename by uuid (bz231187)
vgcreate $vg1 $dev1 $dev3
UUID=$(vgs --noheading -o vg_uuid $vg1)
check_vg_field_ $vg1 vg_uuid $UUID
vgrename $UUID $vg2
check_vg_field_ $vg2 vg_name $vg2
vgremove $vg2
