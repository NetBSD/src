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

test_description='Test for proper escaping of strings in metadata (bz431474)'
privileges_required_=1

. ./test-lib.sh

cleanup_()
{
  test -n "$vg" && {
    vgchange -an "$vg"
    vgremove "$vg"
  } > "$test_dir_/cleanup.log"
  test -n "$d1" && losetup -d "$d1"
  rm -f "$f1"
}

pv_suffix="__\"!@#\$%^&*()'\\\"__"

test_expect_success \
  'set up temp files, loopback devices' \
  'f1=$(pwd)/1 && d1=$(loop_setup_ "$f1") &&
  mv "$d1" "$d1$pv_suffix"                &&
  d1=$d1$pv_suffix'

test_expect_success \
  'pvcreate, vgcreate on filename with backslashed chars' \
  'pvcreate "$d1"                          &&
  vg=$(this_test_)-test-vg-$$              &&
  vgcreate $vg $d1'
  
test_expect_success \
  'no parse errors and VG really exists' \
  'vgs 2>err                                    &&
  grep "Parse error" err;
  status=$?; echo status=$status; test $status -ne 0 &&
  vgs $vg'

test_done
# Local Variables:
# indent-tabs-mode: nil
# End:
