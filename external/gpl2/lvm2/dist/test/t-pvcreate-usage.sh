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

test_description='Test pvcreate option values'

. ./test-utils.sh

aux prepare_devs 4

#COMM 'pvcreate rejects negative setphysicalvolumesize'
not pvcreate --setphysicalvolumesize -1024 $dev1

#COMM 'pvcreate rejects negative metadatasize'
not pvcreate --metadatasize -1024 $dev1

# x. metadatasize 0, defaults to 255
# FIXME: unable to check default value, not in reporting cmds
# should default to 255 according to code
#   check_pv_field_ pv_mda_size 255 
#COMM 'pvcreate accepts metadatasize 0'
pvcreate --metadatasize 0 $dev1
pvremove $dev1

# x. metadatasize too large
# For some reason we allow this, even though there's no room for data?
##COMM  'pvcreate rejects metadatasize too large' 
#not pvcreate --metadatasize 100000000000000 $dev1

#COMM 'pvcreate rejects metadatacopies < 0'
not pvcreate --metadatacopies -1 $dev1

#COMM 'pvcreate accepts metadatacopies = 0, 1, 2'
pvcreate --metadatacopies 0 $dev1 
pvcreate --metadatacopies 1 $dev2 
pvcreate --metadatacopies 2 $dev3 
check_pv_field_ $dev1 pv_mda_count 0 
check_pv_field_ $dev2 pv_mda_count 1 
check_pv_field_ $dev3 pv_mda_count 2 
pvremove $dev1 
pvremove $dev2 
pvremove $dev3

#COMM 'pvcreate rejects metadatacopies > 2'
not pvcreate --metadatacopies 3 $dev1

#COMM 'pvcreate rejects invalid device'
not pvcreate $dev1bogus

#COMM 'pvcreate rejects labelsector < 0'
not pvcreate --labelsector -1 $dev1

#COMM 'pvcreate rejects labelsector > 1000000000000'
not pvcreate --labelsector 1000000000000 $dev1

# other possibilites based on code inspection (not sure how hard)
# x. device too small (min of 512 * 1024 KB)
# x. device filtered out
# x. unable to open /dev/urandom RDONLY
# x. device too large (pe_count > UINT32_MAX)
# x. device read-only
# x. unable to open device readonly
# x. BLKGETSIZE64 fails
# x. set size to value inconsistent with device / PE size

