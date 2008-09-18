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

test_description='Test lvresize command options for validity'
privileges_required_=1

. ./test-lib.sh

cleanup_()
{
  test -n "$vg" && {
    vgchange -an "$vg"
    lvremove -ff "$vg"
    vgremove "$vg"
  } > "$test_dir_/cleanup.log"
  test -n "$d1" && losetup -d "$d1"
  test -n "$d2" && losetup -d "$d2"
  rm -f "$f1" "$f2"
}

test_expect_success \
  'set up temp files, loopback devices, PVs, and a VG' \
  'f1=$(pwd)/1 && d1=$(loop_setup_ "$f1") &&
   f2=$(pwd)/2 && d2=$(loop_setup_ "$f2") &&
   pvcreate $d1 $d2                       &&
   vg=$(this_test_)-test-vg-$$            &&
   vgcreate $vg $d1 $d2'

lv=lvresize-usage-$$

test_expect_success \
  'lvresize normal operation succeeds' \
  'lvcreate -L 64M -n $lv -i2 $vg &&
   lvresize -l +4 $vg/$lv && 
   lvremove -ff $vg'

test_expect_success \
  'lvresize rejects an invalid vgname' \
  'lvcreate -L 64M -n $lv -i2 $vg &&
   lvresize -v -l +4 xxx/$lv &&
   echo status=$?; test $? = 5 &&
   lvremove -ff $vg'

test_done
# Local Variables:
# indent-tabs-mode: nil
# End:
