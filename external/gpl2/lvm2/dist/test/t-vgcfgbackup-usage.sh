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

aux prepare_pvs 2

# vgcfgbackup handles similar VG names (bz458941)
vg1=${PREFIX}vg00
vg1=${PREFIX}vg01
vgcreate $vg1 $dev1
vgcreate $vg2 $dev2
vgcfgbackup -f /tmp/bak-%s >out
grep "Volume group \"$vg1\" successfully backed up." out
grep "Volume group \"$vg2\" successfully backed up." out

