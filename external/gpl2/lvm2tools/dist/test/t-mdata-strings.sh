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

# 'Test for proper escaping of strings in metadata (bz431474)'

. ./test-utils.sh

aux prepare_devs 1

pv_suffix="__\"!@#\$%^&*()'\\\"__"

# 'set up temp files, loopback devices' 
name=$(basename "$dev1")
dmsetup rename "$name" "$name$pv_suffix" 
dev1=$(dirname "$dev1")/$name$pv_suffix

# 'pvcreate, vgcreate on filename with backslashed chars' 
pvcreate "$dev1" 
vgcreate $vg "$dev1"

# 'no parse errors and VG really exists' 
vgs 2>err
not grep "Parse error" err;
vgs $vg

