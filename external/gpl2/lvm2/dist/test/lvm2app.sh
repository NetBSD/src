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
# tests lvm2app library
#

. ./test-utils.sh

aux prepare_devs 2

pvcreate $dev1 $dev2

echo `pwd`
ls -lR `pwd`
$abs_srcdir/api/vgtest $vg1 $dev1 $dev2
