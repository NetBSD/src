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

test_description='Ensure that pvmove diagnoses PE-range values 2^32 and larger.'
privileges_required_=1

. ./test-lib.sh

cleanup_()
{
  test -n "$vg" && {
    vgchange -an "$vg"
    lvremove -ff "$vg"
    vgremove "$vg"
  } > /dev/null
  test -n "$d1" && losetup -d "$d1"
  test -n "$d2" && losetup -d "$d2"
  rm -f "$f1" "$f2"
}

test_expect_success \
  'set up temp files, loopback devices, PVs, VG, LV' \
  'f1=$(pwd)/1 && d1=$(loop_setup_ "$f1") &&
   f2=$(pwd)/2 && d2=$(loop_setup_ "$f2") &&
   pvcreate $d1 $d2              &&
   vg=pvmove-demo-vg-$$          &&
   vgcreate "$vg" $d1 $d2        &&
   lv=lv1                        &&
   lvcreate -L4 -n"$lv" "$vg"'

# Test for the bogus diagnostic reported in BZ 284771
# http://bugzilla.redhat.com/284771.
test_expect_success \
  'run pvmove with an unrecognized LV name to show bad diagnostic' \
  'pvmove -v -nbogus $d1 $d2 2> err
   test $? = 5 &&
   tail -n1 err > out &&
   echo "  Logical volume bogus not found." > expected &&
   diff -u out expected'

# With lvm-2.02.28 and earlier, on a system with 64-bit "long int",
# the PE range parsing code would accept values up to 2^64-1, but would
# silently truncate them to int32_t.  I.e., $d1:$(echo 2^32|bc) would be
# treated just like $d1:0.
test_expect_failure \
  'run the offending pvmove command' \
  'pvmove -v -n$lv $d1:4294967296 $d2'

test_done
# Local Variables:
# indent-tabs-mode: nil
# End:
