# Copyright (C) 2007-2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

. ./test-utils.sh

aux prepare_vg 2

lvcreate -L 10M -n lv -i2 $vg
lvresize -l +4 $vg/lv
lvremove -ff $vg

lvcreate -L 64M -n $lv -i2 $vg
not lvresize -v -l +4 xxx/$lv
# Copyright (C) 2007-2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

. ./test-utils.sh

aux prepare_vg 2

lvcreate -L 10M -n lv -i2 $vg
lvresize -l +4 $vg/lv
lvremove -ff $vg

lvcreate -L 64M -n $lv -i2 $vg
not lvresize -v -l +4 xxx/$lv
