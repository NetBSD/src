#!/bin/sh
# Copyright (C) 2007 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

test_description='Test lvmcache operation'
privileges_required_=1

. ./test-lib.sh

cleanup_()
{
  test -n "$d1" && losetup -d "$d1"
  test -n "$d2" && losetup -d "$d2"
  test -n "$d3" && losetup -d "$d3"
  test -n "$d4" && losetup -d "$d4"
  test -n "$d5" && losetup -d "$d5"
  rm -f "$f1" "$f2" "$f3" "$f4" "$f5"
}

# FIXME: paramaterize lvm1 vs lvm2 metadata; most of these tests should run
# fine with lvm1 metadata as well; for now, just add disks 5 and 6 as lvm1
# metadata
test_expect_success \
  'set up temp files, loopback devices, PVs, vgnames' \
  'f1=$(pwd)/1 && d1=$(loop_setup_ "$f1") &&
   f2=$(pwd)/2 && d2=$(loop_setup_ "$f2") &&
   f3=$(pwd)/3 && d3=$(loop_setup_ "$f3") &&
   f4=$(pwd)/4 && d4=$(loop_setup_ "$f4") &&
   f5=$(pwd)/5 && d5=$(loop_setup_ "$f5") &&
   vg1=$(this_test_)-test-vg1-$$          &&
   vg2=$(this_test_)-test-vg2-$$          &&
   lv1=$(this_test_)-test-lv1-$$          &&
   lv2=$(this_test_)-test-lv2-$$          &&
   lv3=$(this_test_)-test-lv3-$$          &&
   pvcreate $d1 $d2 $d3 $d4 $d5'

#
# Duplicate vg name should cause warnings (but not segfault!)
#
test_expect_success \
  "Duplicate vg name might cause warnings but should succeed" \
  'vgcreate $vg1 $d1 &&
   vgcreate $vg2 $d3 &&
   losetup -d $d1 &&
   vgcreate $vg1 $d2 &&
   losetup $d1 $f1 &&
   pvs &&
   pvs'

test_done

# Local Variables:
# indent-tabs-mode: nil
# End:
