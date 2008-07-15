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

#
# tests basic functionality of read-ahead and ra regressions
#

test_description='Test read-ahead functionality'
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

get_lvs_()
{
   case $(lvs --units s --nosuffix --noheadings -o $1_read_ahead "$vg"/"$lv") in
    *$2) true ;;
    *) false ;;
   esac
}

test_expect_success "set up temp files, loopback devices" \
  'f1=$(pwd)/1 && d1=$(loop_setup_ "$f1") &&
   f2=$(pwd)/2 && d2=$(loop_setup_ "$f2") &&
   f3=$(pwd)/3 && d3=$(loop_setup_ "$f3") &&
   f4=$(pwd)/4 && d4=$(loop_setup_ "$f4") &&
   f5=$(pwd)/5 && d5=$(loop_setup_ "$f5") &&
   vg=$(this_test_)-test-vg-$$            &&
   lv=$(this_test_)-test-lv-$$'

test_expect_success "test various read ahead settings" \
  'pvcreate "$d1"                                    &&
   pvcreate "$d2"                                    &&
   pvcreate "$d3"                                    &&
   pvcreate "$d4"                                    &&
   pvcreate "$d5"                                    &&
   vgcreate -c n "$vg" "$d1" "$d2" "$d3" "$d4" "$d5" &&
   lvcreate -n "$lv" -l 100%FREE -i5 -I256 "$vg"     &&
   lvdisplay "$vg"/"$lv"                             &&
   lvchange -r auto "$vg"/"$lv" || true | grep auto  &&
   get_lvs_ lv auto                                  &&
   get_lvs_ lv_kernel 5120                           &&
   lvchange -r 400 "$vg/$lv"                         &&
   get_lvs_ lv 400                                   &&
   vgremove -f "$vg"'

test_done

# Local Variables:
# indent-tabs-mode: nil
# End:
