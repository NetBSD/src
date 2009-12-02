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

# 'Exercise some lvcreate diagnostics'

. ./test-utils.sh

aux prepare_pvs 4
aux pvcreate --metadatacopies 0 $dev1
vgcreate -cn $vg $devs

# "lvcreate rejects repeated invocation (run 2 times) (bz178216)" 
lvcreate -n $lv -l 4 $vg 
not lvcreate -n $lv -l 4 $vg
lvremove -ff $vg/$lv
# try to remove it again - should fail (but not segfault)
not lvremove -ff $vg/$lv

# "lvcreate rejects a negative stripe_size"
not lvcreate -L 64m -n $lv -i2 --stripesize -4 $vg 2>err;
grep "^  Negative stripesize is invalid\$" err

# 'lvcreate rejects a too-large stripesize'
not lvcreate -L 64m -n $lv -i2 --stripesize 4294967291 $vg 2>err
grep "^  Stripe size cannot be larger than" err

# 'lvcreate w/single stripe succeeds with diagnostics to stdout' 
lvcreate -L 64m -n $lv -i1 --stripesize 4 $vg >out 2>err
grep "^  Redundant stripes argument: default is 1\$" out 
grep "^  Ignoring stripesize argument with single stripe\$" out 
lvdisplay $vg 
lvremove -ff $vg

# 'lvcreate w/default (64KB) stripe size succeeds with diagnostics to stdout'
lvcreate -L 64m -n $lv -i2 $vg > out
grep "^  Using default stripesize" out 
lvdisplay $vg 
check_lv_field_ $vg/$lv stripesize "64.00k"
lvremove -ff $vg

# 'lvcreate rejects an invalid number of stripes' 
not lvcreate -L 64m -n $lv -i129 $vg 2>err
grep "^  Number of stripes (129) must be between 1 and 128\$" err

# The case on lvdisplay output is to verify that the LV was not created.
# 'lvcreate rejects an invalid stripe size'
not lvcreate -L 64m -n $lv -i2 --stripesize 3 $vg 2>err
grep "^  Invalid stripe size" err
case $(lvdisplay $vg) in "") true ;; *) false ;; esac

# Setting max_lv works. (bz490298)
lvremove -ff $vg
vgchange -l 3 $vg
lvcreate -l1 -n $lv1 $vg
lvcreate -l1 -s -n $lv2 $vg/$lv1
lvcreate -l1 -n $lv3 $vg
not lvcreate -l1 -n $lv4 $vg

lvremove -ff $vg/$lv3
lvcreate -l1 -s -n $lv3 $vg/$lv1
not lvcreate -l1 -n $lv4 $vg
not lvcreate -l1 -m1 -n $lv4 $vg

lvremove -ff $vg/$lv3
lvcreate -l1 -m1 -n $lv3 $vg
lvs
vgs -o +max_lv
not lvcreate -l1 -n $lv4 $vg
not lvcreate -l1 -m1 -n $lv4 $vg

lvconvert -m0 $vg/$lv3
lvconvert -m2 -i 1 $vg/$lv3
lvconvert -m1 $vg/$lv3

not vgchange -l 2
vgchange -l 4
vgs $vg

lvremove -ff $vg
vgchange -l 0 $vg

# lvcreate rejects invalid chunksize, accepts between 4K and 512K
# validate origin_size
vgremove -ff $vg
vgcreate -cn $vg $devs
lvcreate -L 32m -n $lv1 $vg
not lvcreate -L 8m -n $lv2 -s --chunksize 3k $vg/$lv1
not lvcreate -L 8m -n $lv2 -s --chunksize 1024k $vg/$lv1
lvcreate -L 8m -n $lv2 -s --chunksize 4k $vg/$lv1
check_lv_field_ $vg/$lv2 chunk_size 4.00k
check_lv_field_ $vg/$lv2 origin_size 32.00m
lvcreate -L 8m -n $lv3 -s --chunksize 512k $vg/$lv1
check_lv_field_ $vg/$lv3 chunk_size 512.00k
check_lv_field_ $vg/$lv3 origin_size 32.00m
lvremove -ff $vg
vgchange -l 0 $vg

# regionsize must be
# - nonzero (bz186013)
# - a power of 2 and a multiple of page size
# - <= size of LV
not lvcreate -L 32m -n $lv -R0 $vg 2>err
grep "Non-zero region size must be supplied." err
not lvcreate -L 32m -n $lv -R 11k $vg
not lvcreate -L 32m -n $lv -R 1k $vg
lvcreate -L 32m -n $lv --regionsize 128m  -m 1 $vg
check_lv_field_ $vg/$lv regionsize "32.00m"
lvremove -ff $vg
lvcreate -L 32m -n $lv --regionsize 4m -m 1 $vg
check_lv_field_ $vg/$lv regionsize "4.00m"
lvremove -ff $vg

# snapshot with virtual origin works
lvcreate -s --virtualoriginsize 64m -L 32m -n $lv1 $vg
lvrename $vg/$lv1 $vg/$lv2
lvcreate -s --virtualoriginsize 64m -L 32m -n $lv1 $vg
lvchange -a n $vg/$lv1
lvremove $vg/$lv1
lvremove -ff $vg
