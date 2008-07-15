#!/bin/sh
# Copyright (C) 2007-2008 Red Hat, Inc. All rights reserved.
# Copyright (C) 2007-2008 NEC Corporation
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

test_description="check namings of mirrored LV"
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

vg=mirror-names-vg-$$
lv1=lv1
lv2=lv2

# ---------------------------------------------------------------------
# Utilities

pv_()
{
  echo "$G_dev_/mapper/pv$1"
}

lv_devices_()
{
  local d
  local lv=$1
  shift
  local devices=$*

  local devs=$(lvs -a -odevices --noheadings $lv | sed 's/([0-9]*)//g' |
               sed 's/ //g' | sed 's/,/ /g')

  for d in $devs; do
    (echo $devices | grep -q $d)  || return 1
    devices=$(echo $devices | sed "s/$d//")
  done

  [ "$(echo $devices | sed 's/ //g')" = "" ]
}

lv_mirror_log_()
{
  local lv=$1

  echo $(lvs -a -omirror_log --noheadings $lv | sed 's/ //g')
}

lv_convert_lv_()
{
  local lv=$1

  echo $(lvs -a -oconvert_lv --noheadings $lv | sed 's/ //g')
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
  lvremove -ff $vg;
  :
}

check_and_cleanup_lvs_()
{
  lvs -a -o+devices $vg &&
  lvremove -ff $vg
}

test_expect_success "check environment setup/cleanup" \
  'prepare_lvs_ &&
   check_and_cleanup_lvs_'

# ---------------------------------------------------------------------
# basic

test_expect_success "init: lvcreate" "prepare_lvs_"

test_expect_success "mirror images are ${lv1}_mimage_x" \
  'lvcreate -l2 -m1 -n $lv1 $vg &&
   lv_devices_ $vg/$lv1 "$lv1"_mimage_0 "$lv1"_mimage_1'

test_expect_success "mirror log is ${lv1}_mlog" \
  'lv_mirror_log_ $vg/$lv1 "$lv1"_mlog'

test_expect_success "cleanup" "check_and_cleanup_lvs_"

# ---------------------------------------------------------------------
# lvrename

test_expect_success "init: lvrename" "prepare_lvs_"

test_expect_success "renamed mirror names: $lv1 to $lv2" \
  'lvcreate -l2 -m1 -n $lv1 $vg &&
   lvrename $vg/$lv1 $vg/$lv2 &&
   lv_devices_ $vg/$lv2 "$lv2"_mimage_0 "$lv2"_mimage_1 &&
   lv_mirror_log_ $vg/$lv2 "$lv2"_mlog'

test_expect_success "cleanup" "check_and_cleanup_lvs_"

# ---------------------------------------------------------------------
# lvconvert

test_expect_success "init: lvconvert" "prepare_lvs_"

test_expect_success "converting mirror names is ${lv1}_mimagetmp_2" \
  'lvcreate -l2 -m1 -n $lv1 $vg &&
   lvconvert -m+1 -i1000 -b $vg/$lv1 &&
   convlv=$(lv_convert_lv_ "$vg/$lv1") &&
   test "$convlv" = "$lv1"_mimagetmp_2 &&
   lv_devices_ $vg/$lv1 "$convlv" "$lv1"_mimage_2 &&
   lv_devices_ "$vg/$convlv" "$lv1"_mimage_0 "$lv1"_mimage_1 &&
   loglv=$(lv_mirror_log_ "$vg/$convlv") &&
   test "$loglv" = "$lv1"_mlog'

test_expect_success "mirror log name after re-adding is ${lv1}_mlog" \
  'lvconvert --mirrorlog core $vg/$lv1 &&
   lvconvert --mirrorlog disk $vg/$lv1 &&
   convlv=$(lv_convert_lv_ "$vg/$lv1") &&
   lv_devices_ $vg/$lv1 "$convlv" "$lv1"_mimage_2 &&
   lv_devices_ "$vg/$convlv" "$lv1"_mimage_0 "$lv1"_mimage_1 &&
   loglv=$(lv_mirror_log_ "$vg/$convlv") &&
   test "$loglv" = "$lv1"_mlog'

test_expect_success "renamed converting mirror names: $lv1 to $lv2" \
  'lvrename $vg/$lv1 $vg/$lv2 &&
   convlv=$(lv_convert_lv_ "$vg/$lv2") &&
   lv_devices_ $vg/$lv2 "$convlv" "$lv2"_mimage_2 &&
   lv_devices_ "$vg/$convlv" "$lv2"_mimage_0 "$lv2"_mimage_1 &&
   loglv=$(lv_mirror_log_ "$vg/$convlv") &&
   test "$loglv" = "$lv2"_mlog'

test_expect_success "cleanup" "check_and_cleanup_lvs_"

# Temporary mirror log should have "_mlogtmp_<n>" suffix
# but currently lvconvert doesn't have an option to add the log.
# If such feature is added in future, a test for that should
# be added.

# ---------------------------------------------------------------------
test_done
