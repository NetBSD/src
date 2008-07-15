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

test_description="ensure that lvcreate's select-by-tag code works"
privileges_required_=1

. ./test-lib.sh

dmsetup_has_dm_devdir_support_ ||
{
  say "Your version of dmsetup lacks support for changing DM_DEVDIR."
  say "Skipping this test"
  exit 0
}

cleanup_()
{
  test -n "$vg" && {
    lvremove -ff $vg
    vgremove $vg
  } > /dev/null
  test -n "$pvs" && {
    pvremove $pvs > /dev/null
    for d in $pvs; do
      dmsetup remove $(basename $d)
    done
  }
  losetup -d $lodev
  rm -f $lofile
}

nr_pvs=3
pvsize=$((200 * 1024 * 2))

test_expect_success \
  'set up temp file and loopback device' \
  'lofile=$(pwd)/lofile && lodev=$(loop_setup_ "$lofile")'

offset=0
pvs=
for n in $(seq 1 $nr_pvs); do
  test_expect_success \
      "create pv$n" \
      'echo "0 $pvsize linear $lodev $offset" > in &&
       dmsetup create pv$n < in'
  offset=$(($offset + $pvsize))
done

for n in $(seq 1 $nr_pvs); do
  pvs="$pvs $G_dev_/mapper/pv$n"
done

test_expect_success \
  "Run this: pvcreate $pvs" \
  'pvcreate $pvs'

vg=lvcreate-pvtags-vg-$$
test_expect_success \
  'set up a VG with tagged PVs' \
  'vgcreate $vg $pvs &&
   pvchange --addtag fast $pvs'

test_expect_success '3 stripes with 3 PVs (selected by tag, @fast) is fine' \
  'lvcreate -l3 -i3 $vg @fast'
test_expect_failure 'too many stripes(4) for 3 PVs' \
  'lvcreate -l4 -i4 $vg @fast'
test_expect_failure '2 stripes is too many with just one PV' \
  'lvcreate -l2 -i2 $vg $G_dev_/mapper/pv1'

test_expect_success 'lvcreate mirror' \
  'lvcreate -l1 -m1 $vg @fast'
test_expect_success 'lvcreate mirror w/corelog' \
  'lvcreate -l1 -m2 --corelog $vg @fast'
test_expect_failure 'lvcreate mirror w/no free PVs' \
  'lvcreate -l1 -m2 $vg @fast'
test_expect_failure 'lvcreate mirror (corelog, w/no free PVs)' \
  'lvcreate -l1 -m3 --corelog $vg @fast'
test_expect_failure 'lvcreate mirror with a single PV arg' \
  'lvcreate -l1 -m1 --corelog $vg $G_dev_/mapper/pv1'

test_done
