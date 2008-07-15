#!/bin/sh
# Copyright (C) 2007-2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

test_description='Test vgmerge operation'
privileges_required_=1

. ./test-lib.sh

cleanup_()
{
  test -n "$d1" && losetup -d "$d1"
  test -n "$d2" && losetup -d "$d2"
  test -n "$d3" && losetup -d "$d3"
  test -n "$d4" && losetup -d "$d4"
  rm -f "$f1" "$f2" "$f3" "$f4"
}

test_expect_success \
  'set up temp files, loopback devices, PVs, vgnames' \
  'f1=$(pwd)/1 && d1=$(loop_setup_ "$f1") &&
   f2=$(pwd)/2 && d2=$(loop_setup_ "$f2") &&
   f3=$(pwd)/3 && d3=$(loop_setup_ "$f3") &&
   f4=$(pwd)/4 && d4=$(loop_setup_ "$f4") &&
   vg1=$(this_test_)-test-vg1-$$          &&
   vg2=$(this_test_)-test-vg2-$$          &&
   lv1=$(this_test_)-test-lv1-$$          &&
   lv2=$(this_test_)-test-lv2-$$          &&
   lv3=$(this_test_)-test-lv3-$$          &&
   pvcreate $d1 $d2 $d3 $d4'

test_expect_success \
  'vgmerge succeeds with single linear LV in source VG' \
  'vgcreate $vg1 $d1 $d2 &&
   vgcreate $vg2 $d3 $d4 &&
   lvcreate -l 4 -n $lv1 $vg1 $d1 &&
   vgchange -an $vg1 &&
   vg_validate_pvlv_counts_ $vg1 2 1 0 &&
   vg_validate_pvlv_counts_ $vg2 2 0 0 &&
   vgmerge $vg2 $vg1 &&
   vg_validate_pvlv_counts_ $vg2 4 1 0 &&
   vgremove -f $vg2'

test_expect_success \
  'vgmerge succeeds with single linear LV in source and destination VG' \
  'vgcreate $vg1 $d1 $d2 &&
   vgcreate $vg2 $d3 $d4 &&
   lvcreate -l 4 -n $lv1 $vg1 &&
   lvcreate -l 4 -n $lv2 $vg2 &&
   vgchange -an $vg1 &&
   vgchange -an $vg2 &&
   vg_validate_pvlv_counts_ $vg1 2 1 0 &&
   vg_validate_pvlv_counts_ $vg2 2 1 0 &&
   vgmerge $vg2 $vg1 &&
   vg_validate_pvlv_counts_ $vg2 4 2 0 &&
   vgremove -f $vg2'

test_expect_success \
  'vgmerge succeeds with linear LV + snapshots in source VG' \
  'vgcreate $vg1 $d1 $d2 &&
   vgcreate $vg2 $d3 $d4 &&
   lvcreate -l 16 -n $lv1 $vg1 &&
   lvcreate -l 4 -s -n $lv2 $vg1/$lv1 &&
   vgchange -an $vg1 &&
   vg_validate_pvlv_counts_ $vg1 2 2 1 &&
   vg_validate_pvlv_counts_ $vg2 2 0 0 &&
   vgmerge $vg2 $vg1 &&
   vg_validate_pvlv_counts_ $vg2 4 2 1 &&
   lvremove -f $vg2/$lv2 &&
   vgremove -f $vg2'

test_expect_success \
  'vgmerge succeeds with mirrored LV in source VG' \
  'vgcreate $vg1 $d1 $d2 $d3 &&
   vgcreate $vg2 $d4 &&
   lvcreate -l 4 -n $lv1 -m1 $vg1 &&
   vgchange -an $vg1 &&
   vg_validate_pvlv_counts_ $vg1 3 1 0 &&
   vg_validate_pvlv_counts_ $vg2 1 0 0 &&
   vgmerge $vg2 $vg1 &&
   vg_validate_pvlv_counts_ $vg2 4 1 0 &&
   lvremove -f $vg2/$lv1 &&
   vgremove -f $vg2'

test_done
# Local Variables:
# indent-tabs-mode: nil
# End:
