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

test_description="ensure that lvconvert for mirrored LV works"
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

vg=mirror-lvconvert-vg-$$
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

mimages_are_redundant_ ()
{
  local vg=$1
  local lv=$vg/$2
  local i

  rm -f out
  for i in $(lvs -odevices --noheadings $lv | sed 's/([^)]*)//g; s/,/ /g'); do
    lvs -a -odevices --noheadings $vg/$i | sed 's/([^)]*)//g; s/,/ /g' | \
      sort | uniq >> out
  done

  # if any duplication is found, it's not redundant
  sort out | uniq -d | grep . && return 1

  return 0
}

lv_is_contiguous_ ()
{
  local lv=$1

  # if the lv has multiple segments, it's not contiguous
  [ $(lvs -a --segments --noheadings $lv | wc -l) -ne 1 ] && return 1

  return 0
}

lv_is_clung_ ()
{
  local lv=$1

  # if any duplication is found, it's not redundant
  [ $(lvs -a -odevices --noheadings $lv | sed 's/([^)]*)//g' | \
        sort | uniq | wc -l) -ne 1 ] && return 1

  return 0
}

mimages_are_contiguous_ ()
{
  local vg=$1
  local lv=$vg/$2
  local i

  for i in $(lvs -odevices --noheadings $lv | sed 's/([^)]*)//g; s/,/ /g'); do
    lv_is_contiguous_ $vg/$i || return 1
  done

  return 0
}

mimages_are_clung_ ()
{
  local vg=$1
  local lv=$vg/$2
  local i

  for i in $(lvs -odevices --noheadings $lv | sed 's/([^)]*)//g; s/,/ /g'); do
    lv_is_clung_ $vg/$i || return 1
  done

  return 0
}

mirrorlog_is_on_()
{
  local lv="$1"_mlog
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

check_mirror_count_()
{
  local lv=$1
  local mirrors=$2
  [ "$mirrors" -eq "$(lvs --noheadings -ostripes $lv)" ]
}

check_mirror_log_()
{
  local lv=$1
  local mlog=$(lvs --noheadings -omirror_log $lv | sed -e 's/ //g')
  [ "$(basename $lv)_mlog" = "$mlog" ]
}

wait_conversion_()
{
  local lv=$1
  while (lvs --noheadings -oattr "$lv" | grep -q '^ *c'); do sleep 1; done
}

check_no_tmplvs_()
{
  local lv=$1
  lvs -a --noheadings -oname $(dirname $lv) > out
  ! grep tmp out
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
# mirrored LV tests

# ---
# add mirror to mirror

test_expect_success "add 1 mirror" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0-1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   check_mirror_log_ $vg/$lv1 &&
   lvconvert -m+1 -i1 $vg/$lv1 $(pv_ 4) &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 3 &&
   mimages_are_redundant_ $vg $lv1 &&
   mirrorlog_is_on_ $vg/$lv1 $(pv_ 3) &&
   check_and_cleanup_lvs_'

test_expect_success "add 2 mirrors" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0-1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   check_mirror_log_ $vg/$lv1 &&
   lvconvert -m+2 -i1 $vg/$lv1 $(pv_ 4) $(pv_ 5) &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 4 &&
   mimages_are_redundant_ $vg $lv1 &&
   mirrorlog_is_on_ $vg/$lv1 $(pv_ 3) &&
   check_and_cleanup_lvs_'

test_expect_success "add 1 mirror to core log mirror" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 --mirrorlog core -n $lv1 $vg $(pv_ 1) $(pv_ 2) &&
   check_mirror_count_ $vg/$lv1 2 &&
   !(check_mirror_log_ $vg/$lv1) &&
   lvconvert -m+1 -i1 --mirrorlog core $vg/$lv1 $(pv_ 4) &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 3 &&
   !(check_mirror_log_ $vg/$lv1) &&
   mimages_are_redundant_ $vg $lv1 &&
   check_and_cleanup_lvs_'

test_expect_success "add 2 mirrors to core log mirror" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 --mirrorlog core -n $lv1 $vg $(pv_ 1) $(pv_ 2) &&
   check_mirror_count_ $vg/$lv1 2 &&
   !(check_mirror_log_ $vg/$lv1) &&
   lvconvert -m+2 -i1 --mirrorlog core $vg/$lv1 $(pv_ 4) $(pv_ 5) &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 4 &&
   !(check_mirror_log_ $vg/$lv1) &&
   mimages_are_redundant_ $vg $lv1 &&
   check_and_cleanup_lvs_'

# ---
# add to converting mirror

test_expect_success "add 1 mirror then add 1 more mirror during conversion" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0-1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   check_mirror_log_ $vg/$lv1 &&
   lvconvert -m+1 -b -i100 $vg/$lv1 $(pv_ 4) &&
   lvconvert -m+1 -i3 $vg/$lv1 $(pv_ 5) &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 4 &&
   mimages_are_redundant_ $vg $lv1 &&
   mirrorlog_is_on_ $vg/$lv1 $(pv_ 3) &&
   check_and_cleanup_lvs_'

# ---
# add mirror and disk log

test_expect_success "add 1 mirror and disk log" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 --mirrorlog core -n $lv1 $vg $(pv_ 1) $(pv_ 2) &&
   check_mirror_count_ $vg/$lv1 2 &&
   !(check_mirror_log_ $vg/$lv1) &&
   lvconvert -m+1 --mirrorlog disk -i1 $vg/$lv1 $(pv_ 4) $(pv_ 3):0-1 &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 3 &&
   check_mirror_log_ $vg/$lv1 &&
   mimages_are_redundant_ $vg $lv1 &&
   mirrorlog_is_on_ $vg/$lv1 $(pv_ 3) &&
   check_and_cleanup_lvs_'

# ---
# check polldaemon restarts

test_expect_success "convert inactive mirror and start polling" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0-1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   lvchange -an $vg/$lv1 &&
   lvconvert -m+1 $vg/$lv1 $(pv_ 4) &&
   lvchange -ay $vg/$lv1 &&
   wait_conversion_ $vg/$lv1 &&
   lvs -a &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_and_cleanup_lvs_'

# ---------------------------------------------------------------------
# removal during conversion

test_expect_success "remove newly added mirror" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0-1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   check_mirror_log_ $vg/$lv1 &&
   lvconvert -m+1 -b -i100 $vg/$lv1 $(pv_ 4) &&
   lvconvert -m-1 $vg/$lv1 $(pv_ 4) &&
   wait_conversion_ $vg/$lv1 &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   mimages_are_redundant_ $vg $lv1 &&
   mirrorlog_is_on_ $vg/$lv1 $(pv_ 3) &&
   check_and_cleanup_lvs_'

test_expect_success "remove one of newly added mirrors" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0-1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   check_mirror_log_ $vg/$lv1 &&
   lvconvert -m+2 -b -i100 $vg/$lv1 $(pv_ 4) $(pv_ 5) &&
   lvconvert -m-1 $vg/$lv1 $(pv_ 4) &&
   lvconvert -i1 $vg/$lv1 &&
   wait_conversion_ $vg/$lv1 &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 3 &&
   mimages_are_redundant_ $vg $lv1 &&
   mirrorlog_is_on_ $vg/$lv1 $(pv_ 3) &&
   check_and_cleanup_lvs_'

test_expect_success "remove from original mirror (the original is still mirror)" \
  'prepare_lvs_ &&
   lvcreate -l2 -m2 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 5) $(pv_ 3):0-1 &&
   check_mirror_count_ $vg/$lv1 3 &&
   check_mirror_log_ $vg/$lv1 &&
   lvconvert -m+1 -b -i100 $vg/$lv1 $(pv_ 4) &&
   lvconvert -m-1 $vg/$lv1 $(pv_ 2) &&
   lvconvert -i1 $vg/$lv1 &&
   wait_conversion_ $vg/$lv1 &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 3 &&
   mimages_are_redundant_ $vg $lv1 &&
   mirrorlog_is_on_ $vg/$lv1 $(pv_ 3) &&
   check_and_cleanup_lvs_'

test_expect_success "remove from original mirror (the original becomes linear)" \
  'prepare_lvs_ &&
   lvcreate -l2 -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0-1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   check_mirror_log_ $vg/$lv1 &&
   lvconvert -m+1 -b -i100 $vg/$lv1 $(pv_ 4) &&
   lvconvert -m-1 $vg/$lv1 $(pv_ 2) &&
   lvconvert -i1 $vg/$lv1 &&
   wait_conversion_ $vg/$lv1 &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 2 &&
   mimages_are_redundant_ $vg $lv1 &&
   mirrorlog_is_on_ $vg/$lv1 $(pv_ 3) &&
   check_and_cleanup_lvs_'

test_expect_success "rhbz440405: lvconvert -m0 incorrectly fails if all PEs allocated" \
  'prepare_lvs_ &&
   lvcreate -l`pvs --noheadings -ope_count $(pv_ 1)` -m1 -n $lv1 $vg $(pv_ 1) $(pv_ 2) $(pv_ 3):0 &&
   check_mirror_count_ $vg/$lv1 2 &&
   check_mirror_log_ $vg/$lv1 &&
   lvconvert -m0 $vg/$lv1 $(pv_ 1) &&
   check_no_tmplvs_ $vg/$lv1 &&
   check_mirror_count_ $vg/$lv1 1 &&
   check_and_cleanup_lvs_'

# ---------------------------------------------------------------------

test_done
