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

test_description='Exercise some vgcreate diagnostics'
privileges_required_=1

. ./test-lib.sh

cleanup_()
{
  test -n "$d1" && losetup -d "$d1"
  test -n "$d2" && losetup -d "$d2"
  rm -f "$f1" "$f2"
}

test_expect_success \
  'set up temp files, loopback devices, PVs, vgname' \
  'f1=$(pwd)/1 && d1=$(loop_setup_ "$f1") &&
   f2=$(pwd)/2 && d2=$(loop_setup_ "$f2") &&
   vg=$(this_test_)-test-vg-$$            &&
   pvcreate $d1 $d2'

lv=vgcreate-usage-$$

test_expect_success \
  'vgcreate accepts 8.00M physicalextentsize for VG' \
  'vgcreate $vg --physicalextentsize 8.00M $d1 $d2 &&
   check_vg_field_ $vg vg_extent_size 8.00M &&
   vgremove $vg'

test_expect_success \
  'vgcreate accepts smaller (128) maxlogicalvolumes for VG' \
  'vgcreate $vg --maxlogicalvolumes 128 $d1 $d2 &&
   check_vg_field_ $vg max_lv 128 &&
   vgremove $vg'

test_expect_success \
  'vgcreate accepts smaller (128) maxphysicalvolumes for VG' \
  'vgcreate $vg --maxphysicalvolumes 128 $d1 $d2 &&
   check_vg_field_ $vg max_pv 128 &&
   vgremove $vg'

test_expect_success \
  'vgcreate rejects a zero physical extent size' \
  'vgcreate --physicalextentsize 0 $vg $d1 $d2 2>err;
   status=$?; echo status=$status; test $status = 3 &&
   grep "^  Physical extent size may not be zero\$" err'

test_expect_success \
  'vgcreate rejects "inherit" allocation policy' \
  'vgcreate --alloc inherit $vg $d1 $d2 2>err;
   status=$?; echo status=$status; test $status = 3 &&
   grep "^  Volume Group allocation policy cannot inherit from anything\$" err'

test_expect_success \
  'vgcreate rejects vgname "."' \
  'vg=.; vgcreate $vg $d1 $d2 2>err;
   status=$?; echo status=$status; test $status = 3 &&
   grep "New volume group name \"$vg\" is invalid\$" err'

test_expect_success \
  'vgcreate rejects vgname greater than 128 characters' \
  'vginvalid=thisnameisridiculouslylongtotestvalidationcodecheckingmaximumsizethisiswhathappenswhenprogrammersgetboredandorarenotcreativedonttrythisathome;
   vgcreate $vginvalid $d1 $d2 2>err;
   status=$?; echo status=$status; test $status = 3 &&
   grep "New volume group name \"$vginvalid\" is invalid\$" err'

test_expect_success \
  'vgcreate rejects already existing vgname "/tmp/$vg"' \
  'touch /tmp/$vg; vgcreate $vg $d1 $d2 2>err;
   status=$?; echo status=$status; test $status = 3 &&
   grep "New volume group name \"$vg\" is invalid\$" err'

# FIXME: Not sure why this fails
#test_expect_success \
#  'vgcreate rejects MaxLogicalVolumes > 255' \
#  'vgcreate --metadatatype 1 --maxlogicalvolumes 1024 $vg $d1 $d2 2>err;
#   cp err save;
#   status=$?; echo status=$status; test $status = 3 &&
#   grep "^  Number of volumes may not exceed 255\$" err'

test_done
# Local Variables:
# indent-tabs-mode: nil
# End:
