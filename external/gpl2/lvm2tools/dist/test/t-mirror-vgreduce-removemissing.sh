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

test_description="ensure that 'vgreduce --removemissing' works on mirrored LV"
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

vg=mirror-vgreduce-removemissing-vg-$$
lv1=lv1
lv2=lv2
lv3=lv3

# ---------------------------------------------------------------------
# Utilities

pv_()
{
  echo "$G_dev_/mapper/pv$1"
}

fail_pv_()
{
  local map
  local p
  for p in $*; do
    map=$(basename $p)
    echo "Fail $map"
    [ -e map_$map ] && return
    dmsetup table "$map" > "map_$map"
    echo "0 $pvsize error" | dmsetup load "$map"
    dmsetup resume "$map"
  done
}

recover_pv_()
{
  local map
  local p

  for p in $*; do
    map=$(basename $p)
    echo "Recover $map"
    [ ! -e "map_$map" ] && return
    cat "map_$map" | dmsetup load "$map"
    dmsetup resume "$map"
    rm "map_$map"
  done
}

lv_is_on_ ()
{
  local lv=$vg/$1
  shift
  local pvs=$*

  echo "Check if $lv is exactly on PVs $pvs"
  rm -f out1 out2
  echo $pvs | sed 's/ /\n/g' | sort | uniq > out1

  lvs -a -odevices --noheadings $lv | \
    sed 's/([^)]*)//g; s/[ ,]/\n/g' | sort | uniq > out2

  diff --ignore-blank-lines out1 out2
}

mimages_are_on_ ()
{
  local lv=$1
  shift
  local pvs=$*
  local mimages
  local i

  echo "Check if mirror images of $lv are on PVs $pvs"
  rm -f out1 out2
  echo $pvs | sed 's/ /\n/g' | sort | uniq > out1

  mimages=$(lvs --noheadings -a -o lv_name $vg | grep "${lv}_mimage_" | \
             sed 's/\[//g; s/\]//g')
  for i in $mimages; do
    echo "Checking $vg/$i"
    lvs -a -odevices --noheadings $vg/$i | \
      sed 's/([^)]*)//g; s/ //g; s/,/ /g' | sort | uniq >> out2
  done

  diff --ignore-blank-lines out1 out2
}

mirrorlog_is_on_()
{
  local lv="$1"_mlog
  shift
  lv_is_on_ $lv $*
}

lv_is_linear_()
{
  echo "Check if $1 is linear LV (i.e. not a mirror)"
  lvs -o stripes,attr --noheadings $vg/$1 | sed 's/ //g' | grep -q '^1-'
}

rest_pvs_()
{
  local index=$1
  local num=$2
  local rem=""
  local n

  for n in $(seq 1 $(($index - 1))) $(seq $((index + 1)) $num); do
    rem="$rem $(pv_ $n)"
  done

  echo "$rem"
}

# ---------------------------------------------------------------------
# Initialize PVs and VGs

test_expect_success \
  'set up temp file and loopback device' \
  'lofile=$(pwd)/lofile && lodev=$(loop_setup_ "$lofile")'

offset=1
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
  lvremove -ff $vg;
  :
}

check_and_cleanup_lvs_()
{
  lvs -a -o+devices $vg &&
  lvremove -ff $vg
}

recover_vg_()
{
  recover_pv_ $* &&
  pvcreate -ff $* &&
  vgextend $vg $* &&
  check_and_cleanup_lvs_
}

test_expect_success "check environment setup/cleanup" \
  'prepare_lvs_ &&
   check_and_cleanup_lvs_'

# ---------------------------------------------------------------------
# one of mirror images has failed

test_expect_success "basic: fail the 2nd mirror image of 2-way mirrored LV" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0-1 &&
   lvchange -an $vg/$lv1 &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) &&
   mirrorlog_is_on_ $lv1 $(pv_ 3) &&
   fail_pv_ $(pv_ 2) &&
   vgreduce --removemissing $vg &&
   lv_is_linear_ $lv1 &&
   lv_is_on_ $lv1 $(pv_ 1)'
test_expect_success "cleanup" \
  'recover_vg_ $(pv_ 2)'

# ---------------------------------------------------------------------
# LV has 3 images in flat,
# 1 out of 3 images fails

# test_3way_mirror_fail_1_ <PV# to fail>
test_3way_mirror_fail_1_()
{
   local index=$1

   lvcreate -l2 -m2 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 4):0-1 &&
   lvchange -an $vg/$lv1 &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) &&
   mirrorlog_is_on_ $lv1 $(pv_ 4) &&
   fail_pv_ $(pv_ $index) &&
   vgreduce --removemissing $vg &&
   lvs -a -o+devices $vg &&
   mimages_are_on_ $lv1 $(rest_pvs_ $index 3) &&
   mirrorlog_is_on_ $lv1 $(pv_ 4)
}

for n in $(seq 1 3); do
  test_expect_success "fail mirror image $(($n - 1)) of 3-way mirrored LV" \
    "prepare_lvs_ &&
     test_3way_mirror_fail_1_ $n"
  test_expect_success "cleanup" \
    "recover_vg_ $(pv_ $n)"
done

# ---------------------------------------------------------------------
# LV has 3 images in flat,
# 2 out of 3 images fail

# test_3way_mirror_fail_2_ <PV# NOT to fail>
test_3way_mirror_fail_2_()
{
   local index=$1

   lvcreate -l2 -m2 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 4):0-1 &&
   lvchange -an $vg/$lv1 &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) &&
   mirrorlog_is_on_ $lv1 $(pv_ 4) &&
   fail_pv_ $(rest_pvs_ $index 3) &&
   vgreduce --removemissing $vg &&
   lvs -a -o+devices $vg &&
   lv_is_linear_ $lv1 &&
   lv_is_on_ $lv1 $(pv_ $index)
}

for n in $(seq 1 3); do
  test_expect_success "fail mirror images other than mirror image $(($n - 1)) of 3-way mirrored LV" \
    "prepare_lvs_ &&
     test_3way_mirror_fail_2_ $n"
  test_expect_success "cleanup" \
    "recover_vg_ $(rest_pvs_ $n 3)"
done

# ---------------------------------------------------------------------
# LV has 4 images, 1 of them is in the temporary mirror for syncing.
# 1 out of 4 images fails

# test_3way_mirror_plus_1_fail_1_ <PV# to fail>
test_3way_mirror_plus_1_fail_1_()
{
   local index=$1

   lvcreate -l2 -m2 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   lvconvert -m+1 $vg/$lv1 $(pv_ 4) &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   fail_pv_ $(pv_ $index) &&
   vgreduce --removemissing $vg &&
   lvs -a -o+devices $vg &&
   mimages_are_on_ $lv1 $(rest_pvs_ $index 4) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5)
}

for n in $(seq 1 4); do
  test_expect_success "fail mirror image $(($n - 1)) of 4-way (1 converting) mirrored LV" \
    "prepare_lvs_ &&
     test_3way_mirror_plus_1_fail_1_ $n"
  test_expect_success "cleanup" \
    "recover_vg_ $(pv_ $n)"
done

# ---------------------------------------------------------------------
# LV has 4 images, 1 of them is in the temporary mirror for syncing.
# 3 out of 4 images fail

# test_3way_mirror_plus_1_fail_3_ <PV# NOT to fail>
test_3way_mirror_plus_1_fail_3_()
{
   local index=$1

   lvcreate -l2 -m2 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   lvconvert -m+1 $vg/$lv1 $(pv_ 4) &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   fail_pv_ $(rest_pvs_ $index 4) &&
   vgreduce --removemissing $vg &&
   lvs -a -o+devices $vg &&
   (mimages_are_on_ $lv1 $(pv_ $index) || lv_is_on_ $lv1 $(pv_ $index)) &&
   ! mirrorlog_is_on_ $lv1 $(pv_ 5)
}

for n in $(seq 1 4); do
  test_expect_success "fail mirror images other than mirror image $(($n - 1)) of 4-way (1 converting) mirrored LV" \
    "prepare_lvs_ &&
     test_3way_mirror_plus_1_fail_3_ $n"
  test_expect_success "cleanup" \
    "recover_vg_ $(rest_pvs_ $n 4)"
done

# ---------------------------------------------------------------------
# LV has 4 images, 2 of them are in the temporary mirror for syncing.
# 1 out of 4 images fail

# test_2way_mirror_plus_2_fail_1_ <PV# to fail>
test_2way_mirror_plus_2_fail_1_()
{
   local index=$1

   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   lvconvert -m+2 $vg/$lv1 $(pv_ 3) $(pv_ 4) &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   fail_pv_ $(pv_ $index) &&
   vgreduce --removemissing $vg &&
   lvs -a -o+devices $vg &&
   mimages_are_on_ $lv1 $(rest_pvs_ $index 4) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5)
}

for n in $(seq 1 4); do
  test_expect_success "fail mirror image $(($n - 1)) of 4-way (2 converting) mirrored LV" \
    "prepare_lvs_ &&
     test_2way_mirror_plus_2_fail_1_ $n"
  test_expect_success "cleanup" \
    "recover_vg_ $(pv_ $n)"
done

# ---------------------------------------------------------------------
# LV has 4 images, 2 of them are in the temporary mirror for syncing.
# 3 out of 4 images fail

# test_2way_mirror_plus_2_fail_3_ <PV# NOT to fail>
test_2way_mirror_plus_2_fail_3_()
{
   local index=$1

   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   lvconvert -m+2 $vg/$lv1 $(pv_ 3) $(pv_ 4) &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) $(pv_ 4) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   fail_pv_ $(rest_pvs_ $index 4) &&
   vgreduce --removemissing $vg &&
   lvs -a -o+devices $vg &&
   (mimages_are_on_ $lv1 $(pv_ $index) || lv_is_on_ $lv1 $(pv_ $index)) &&
   ! mirrorlog_is_on_ $lv1 $(pv_ 5)
}

for n in $(seq 1 4); do
  test_expect_success "fail mirror images other than mirror image $(($n - 1)) of 4-way (2 converting) mirrored LV" \
    "prepare_lvs_ &&
     test_2way_mirror_plus_2_fail_3_ $n"
  test_expect_success "cleanup" \
    "recover_vg_ $(rest_pvs_ $n 4)"
done

# ---------------------------------------------------------------------
# log device is gone (flat mirror and stacked mirror)

test_expect_success "fail mirror log of 2-way mirrored LV" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   fail_pv_ $(pv_ 5) &&
   vgreduce --removemissing $vg &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) &&
   ! mirrorlog_is_on_ $lv1 $(pv_ 5)'
test_expect_success "cleanup" \
  "recover_vg_ $(pv_ 5)"

test_expect_success "fail mirror log of 3-way (1 converting) mirrored LV" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   lvconvert -m+1 $vg/$lv1 $(pv_ 3) &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   fail_pv_ $(pv_ 5) &&
   vgreduce --removemissing $vg &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) &&
   ! mirrorlog_is_on_ $lv1 $(pv_ 5)'
test_expect_success "cleanup" \
  "recover_vg_ $(pv_ 5)"

# ---------------------------------------------------------------------
# all images are gone (flat mirror and stacked mirror)

test_expect_success "fail all mirror images of 2-way mirrored LV" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   fail_pv_ $(pv_ 1) $(pv_ 2) &&
   vgreduce --removemissing $vg &&
   ! lvs $vg/$lv1'
test_expect_success "cleanup" \
  "recover_vg_ $(pv_ 1) $(pv_ 2)"

test_expect_success "fail all mirror images of 3-way (1 converting) mirrored LV" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   lvconvert -m+1 $vg/$lv1 $(pv_ 3) &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) $(pv_ 3) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   fail_pv_ $(pv_ 1) $(pv_ 2) $(pv_ 3) &&
   vgreduce --removemissing $vg &&
   ! lvs $vg/$lv1'
test_expect_success "cleanup" \
  "recover_vg_ $(pv_ 1) $(pv_ 2) $(pv_ 3)"

# ---------------------------------------------------------------------
# Multiple LVs

test_expect_success "fail a mirror image of one of mirrored LV" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   lvcreate -l2 -m1 -n $lv2 $vg $(pv_ 3) $(pv_ 4) $(pv_ 5):1-1 &&
   lvchange -an $vg/$lv2 &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) &&
   mimages_are_on_ $lv2 $(pv_ 3) $(pv_ 4) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   mirrorlog_is_on_ $lv2 $(pv_ 5) &&
   fail_pv_ $(pv_ 2) &&
   vgreduce --removemissing $vg &&
   mimages_are_on_ $lv2 $(pv_ 3) $(pv_ 4) &&
   mirrorlog_is_on_ $lv2 $(pv_ 5) &&
   lv_is_linear_ $lv1 &&
   lv_is_on_ $lv1 $(pv_ 1)'
test_expect_success "cleanup" \
  "recover_vg_ $(pv_ 2)"

test_expect_success "fail mirror images, one for each mirrored LV" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   lvcreate -l2 -m1 -n $lv2 $vg $(pv_ 3) $(pv_ 4) $(pv_ 5):1-1 &&
   lvchange -an $vg/$lv2 &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) &&
   mimages_are_on_ $lv2 $(pv_ 3) $(pv_ 4) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   mirrorlog_is_on_ $lv2 $(pv_ 5) &&
   fail_pv_ $(pv_ 2) &&
   fail_pv_ $(pv_ 4) &&
   vgreduce --removemissing $vg &&
   lv_is_linear_ $lv1 &&
   lv_is_on_ $lv1 $(pv_ 1) &&
   lv_is_linear_ $lv2 &&
   lv_is_on_ $lv2 $(pv_ 3)'
test_expect_success "cleanup" \
  "recover_vg_ $(pv_ 2) $(pv_ 4)"

# ---------------------------------------------------------------------
# no failure

test_expect_success "no failures" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5):0-1 &&
   lvchange -an $vg/$lv1 &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5) &&
   vgreduce --removemissing $vg &&
   mimages_are_on_ $lv1 $(pv_ 1) $(pv_ 2) &&
   mirrorlog_is_on_ $lv1 $(pv_ 5)'
test_expect_success "cleanup" \
  'check_and_cleanup_lvs_'

# ---------------------------------------------------------------------

test_done
