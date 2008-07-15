#!/bin/sh
# Copyright (C) 2007 Red Hat, Inc. All rights reserved.
# Copyright (C) 2007 NEC Corporation
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

test_description="ensure that pvmove works with basic options"
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

# ---------------------------------------------------------------------
# config

nr_pvs=5
pvsize=$((80 * 1024 * 2))

vg=pvmove-basic-vg-$$
lv1=lv1
lv2=lv2
lv3=lv3

# ---------------------------------------------------------------------
# Utilities

pv_()
{
  echo "$G_dev_/mapper/pv$1"
}

lvdev_()
{
  echo "$G_dev_/$1/$2"
}

lv_is_on_()
{
  local lv=$1
  shift 1
  lvs -a -odevices --noheadings $lv | sed 's/,/\n/g' > out
  for d in $*; do grep "$d(" out || return 1; done
  for d in $*; do grep -v "$d(" out > out2; mv out2 out; done
  grep . out && return 1
  return 0
}

save_dev_sum_()
{
  mkfs.ext3 $1 > /dev/null &&
  md5sum $1 > md5.$(basename $1)
}

check_dev_sum_()
{
  md5sum $1 > md5.tmp && cmp md5.$(basename $1) md5.tmp
}

# ---------------------------------------------------------------------
# Initialize PVs and VGs

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
  pvs="$pvs $(pv_ $n)"
done

test_expect_success \
  "Run this: pvcreate $pvs" \
  'pvcreate $pvs'

test_expect_success \
  'set up a VG' \
  'vgcreate $vg $pvs'

# ---------------------------------------------------------------------
# Common environment setup/cleanup for each sub testcases

prepare_lvs_()
{
  lvcreate -l2 -n $lv1 $vg $(pv_ 1) &&
    lv_is_on_ $vg/$lv1 $(pv_ 1) &&
  lvcreate -l9 -i3 -n $lv2 $vg $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
    lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
  lvextend -l+2 $vg/$lv1 $(pv_ 2) &&
    lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 2) &&
  lvextend -l+2 $vg/$lv1 $(pv_ 3) &&
    lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) &&
  lvextend -l+2 $vg/$lv1 $(pv_ 1) &&
    lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 1) &&
  lvcreate -l1 -n $lv3 $vg $(pv_ 2) &&
    lv_is_on_ $vg/$lv3 $(pv_ 2) &&
  save_dev_sum_ $(lvdev_ $vg $lv1) &&
  save_dev_sum_ $(lvdev_ $vg $lv2) &&
  save_dev_sum_ $(lvdev_ $vg $lv3) &&
  lvs -a -o devices --noheadings $vg/$lv1 > lv1_devs &&
  lvs -a -o devices --noheadings $vg/$lv2 > lv2_devs &&
  lvs -a -o devices --noheadings $vg/$lv3 > lv3_devs
}

lv_not_changed_()
{
  lvs -a -o devices --noheadings $1 > out &&
  diff $(basename $1)_devs out
}

check_and_cleanup_lvs_()
{
  lvs -a -o+devices $vg &&
  check_dev_sum_ $(lvdev_ $vg $lv1) &&
  check_dev_sum_ $(lvdev_ $vg $lv2) &&
  check_dev_sum_ $(lvdev_ $vg $lv3) &&
  lvs -a -o name $vg > out && ! grep ^pvmove out &&
  lvremove -ff $vg
}

test_expect_success "check environment setup/cleanup" \
  'prepare_lvs_ &&
   check_and_cleanup_lvs_'

# ---------------------------------------------------------------------
# pvmove tests

# ---
# filter by LV

test_expect_success "only specified LV is moved: from pv2 to pv5 only for lv1" \
  'prepare_lvs_ &&
   pvmove -i1 -n $vg/$lv1 $(pv_ 2) $(pv_ 5) &&
   lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 5) $(pv_ 3) $(pv_ 1) &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

# ---
# segments in a LV

test_expect_success "the 1st seg of 3-segs LV is moved: from pv1 of lv1 to pv4" \
  'prepare_lvs_ &&
   pvmove -i1 -n $vg/$lv1 $(pv_ 1) $(pv_ 4) &&
   lv_is_on_ $vg/$lv1 $(pv_ 4) $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "the 2nd seg of 3-segs LV is moved: from pv2 of lv1 to pv4" \
  'prepare_lvs_ &&
   pvmove -i1 -n $vg/$lv1 $(pv_ 2) $(pv_ 4) &&
   lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 4) $(pv_ 3) $(pv_ 1) &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "the 3rd seg of 3-segs LV is moved: from pv3 of lv1 to pv4" \
  'prepare_lvs_ &&
   pvmove -i1 -n $vg/$lv1 $(pv_ 3) $(pv_ 4) &&
   lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 2) $(pv_ 4) $(pv_ 1) &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

# ---
# multiple LVs matching

test_expect_success "1 out of 3 LVs is moved: from pv4 to pv5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 4) $(pv_ 5) &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 3) $(pv_ 5) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "2 out of 3 LVs are moved: from pv3 to pv5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 3) $(pv_ 5) &&
   lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 2) $(pv_ 5) $(pv_ 1) &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 5) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "3 out of 3 LVs are moved: from pv2 to pv5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 2) $(pv_ 5) &&
   lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 5) $(pv_ 3) $(pv_ 1) &&
   lv_is_on_ $vg/$lv2 $(pv_ 5) $(pv_ 3) $(pv_ 4) &&
   lv_is_on_ $vg/$lv3 $(pv_ 5) &&
   check_and_cleanup_lvs_'

# ---
# areas of striping

test_expect_success "move the 1st stripe: from pv2 of lv2 to pv1" \
  'prepare_lvs_ &&
   pvmove -i1 -n $vg/$lv2 $(pv_ 2) $(pv_ 1) &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 1) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "move the 2nd stripe: from pv3 of lv2 to pv1" \
  'prepare_lvs_ &&
   pvmove -i1 -n $vg/$lv2 $(pv_ 3) $(pv_ 1) &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 1) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "move the 3rd stripe: from pv4 of lv2 to pv1" \
  'prepare_lvs_ &&
   pvmove -i1 -n $vg/$lv2 $(pv_ 4) $(pv_ 1) &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 3) $(pv_ 1) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

# ---
# partial segment match (source segment splitted)

test_expect_success "match to the start of segment:from pv2:0-0 to pv5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 2):0-0 $(pv_ 5) &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 5) $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "match to the middle of segment: from pv2:1-1 to pv5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 2):1-1 $(pv_ 5) &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 5) $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "match to the end of segment: from pv2:2-2 to pv5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 2):2-2 $(pv_ 5) &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 5) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

# ---
# destination segment splitted

test_expect_success "no destination split: from pv2:0-2 to pv5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 2):0-2 $(pv_ 5) &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 5) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "destination split into 2: from pv2:0-2 to pv5:5-5 and pv4:5-6" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 2):0-2 $(pv_ 5):5-5 $(pv_ 4):5-6 &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 5) $(pv_ 4) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "destination split into 3: from pv2:0-2 to {pv3,4,5}:5-5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 2):0-2 $(pv_ 3):5-5 $(pv_ 4):5-5 $(pv_ 5):5-5 &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 3) $(pv_ 4) $(pv_ 5) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

# ---
# alloc policy (anywhere, contiguous) with both success and failure cases

test_expect_failure "alloc normal on same PV for source and destination: from pv3:0-2 to pv3:5-7" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 3):0-2 $(pv_ 3):5-7'
test_expect_success "(cleanup previous test)" \
  'lv_not_changed_ $vg/$lv1 &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "alloc anywhere on same PV for source and destination: from pv3:0-2 to pv3:5-7" \
  'prepare_lvs_ &&
   pvmove -i1 --alloc anywhere $(pv_ 3):0-2 $(pv_ 3):5-7 &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "alloc anywhere but better area available: from pv3:0-2 to pv3:5-7 or pv5:5-6,pv4:5-5" \
  'prepare_lvs_ &&
   pvmove -i1 --alloc anywhere $(pv_ 3):0-2 $(pv_ 3):5-7 $(pv_ 5):5-6 $(pv_ 4):5-5 &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 5) $(pv_ 4) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_failure "alloc contiguous but area not available: from pv2:0-2 to pv5:5-5 and pv4:5-6" \
  'prepare_lvs_ &&
   pvmove -i1 --alloc contiguous $(pv_ 2):0-2 $(pv_ 5):5-5 $(pv_ 4):5-6'
test_expect_success "(cleanup previous test)" \
  'lv_not_changed_ $vg/$lv1 &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_success "alloc contiguous and contiguous area available: from pv2:0-2 to pv5:0-0,pv5:3-5 and pv4:5-6" \
  'prepare_lvs_ &&
   pvmove -i1 --alloc contiguous $(pv_ 2):0-2 $(pv_ 5):0-0 $(pv_ 5):3-5 $(pv_ 4):5-6 &&
   lv_not_changed_ $vg/$lv1 &&
   lv_is_on_ $vg/$lv2 $(pv_ 5) $(pv_ 3) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

# ---
# multiple segments in a LV

test_expect_success "multiple source LVs: from pv3 to pv5" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 3) $(pv_ 5) &&
   lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 2) $(pv_ 5) &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 5) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

# ---
# move inactive LV

test_expect_success "move inactive LV: from pv2 to pv5" \
  'prepare_lvs_ &&
   lvchange -an $vg/$lv1 &&
   lvchange -an $vg/$lv3 &&
   pvmove -i1 $(pv_ 2) $(pv_ 5) &&
   lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 5) $(pv_ 3) &&
   lv_is_on_ $vg/$lv2 $(pv_ 5) $(pv_ 3) $(pv_ 4) &&
   lv_is_on_ $vg/$lv3 $(pv_ 5) &&
   check_and_cleanup_lvs_'

# ---
# other failure cases

test_expect_failure "no PEs to move: from pv3 to pv1" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 3) $(pv_ 1) &&
   pvmove -i1 $(pv_ 3) $(pv_ 1)'
test_expect_success "(cleanup previous test)" \
  'lv_is_on_ $vg/$lv1 $(pv_ 1) $(pv_ 2) $(pv_ 1) &&
   lv_is_on_ $vg/$lv2 $(pv_ 2) $(pv_ 1) $(pv_ 4) &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_failure "no space available: from pv2:0-0 to pv1:0-0" \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 2):0-0 $(pv_ 1):0-0'
test_expect_success "(cleanup previous test)" \
  'lv_not_changed_ $vg/$lv1 &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_failure 'same source and destination: from pv1 to pv1' \
  'prepare_lvs_ &&
   pvmove -i1 $(pv_ 1) $(pv_ 1)'
test_expect_success "(cleanup previous test)" \
  'lv_not_changed_ $vg/$lv1 &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

test_expect_failure "sum of specified destination PEs is large enough, but it includes source PEs and the free PEs are not enough" \
  'prepare_lvs_ &&
   pvmove --alloc anywhere $(pv_ 1):0-2 $(pv_ 1):0-2 $(pv_ 5):0-0 2> err'
test_expect_success "(cleanup previous test)" \
  'grep "Insufficient free space" err &&
   lv_not_changed_ $vg/$lv1 &&
   lv_not_changed_ $vg/$lv2 &&
   lv_not_changed_ $vg/$lv3 &&
   check_and_cleanup_lvs_'

# ---------------------------------------------------------------------

test_expect_success "pvmove abort" \
  'prepare_lvs_ &&
   pvmove -i100 -b $(pv_ 1) $(pv_ 3) &&
   pvmove --abort &&
   check_and_cleanup_lvs_'

test_done
