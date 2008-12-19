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

aux prepare_pvs 5

vgcreate $vg1 $dev1
vgcreate $vg2 $dev3

disable_dev $dev1
pvscan
vgcreate $vg1 $dev2
enable_dev $dev1
pvs
pvs
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

aux prepare_pvs 5

vgcreate $vg1 $dev1
vgcreate $vg2 $dev3

disable_dev $dev1
pvscan
vgcreate $vg1 $dev2
enable_dev $dev1
pvs
pvs
