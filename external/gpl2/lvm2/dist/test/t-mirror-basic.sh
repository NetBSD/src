#!/bin/sh
# Copyright (C) 2008 Red Hat, Inc. All rights reserved.
# Copyright (C) 2007 NEC Corporation
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

test_description="ensure that basic operations on mirrored LV works"

. ./test-utils.sh

dmsetup_has_dm_devdir_support_ || exit 200

# ---------------------------------------------------------------------
# Utilities

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
  for d in $*; do grep "$d(" out; done
  for d in $*; do ! grep -v "$d(" out; done
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

aux prepare_vg 5 80

# ---------------------------------------------------------------------
# Common environment setup/cleanup for each sub testcases

prepare_lvs_()
{
  lvremove -ff $vg;
	if dmsetup table|grep $vg; then
		echo "ERROR: lvremove did leave some some mappings in DM behind!"
		return 1
	fi
  :
}

check_and_cleanup_lvs_()
{
  lvs -a -o+devices $vg
  lvremove -ff $vg
	if dmsetup table|grep $vg; then
		echo "ERROR: lvremove did leave some some mappings in DM behind!"
		return 1
	fi
}

#COMM "check environment setup/cleanup"
prepare_lvs_ 
check_and_cleanup_lvs_

# ---------------------------------------------------------------------
# mirrored LV tests

# ---
# create

#COMM "create 2-way mirror with disklog from 3 PVs"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

#COMM "create 2-way mirror with corelog from 2 PVs"
prepare_lvs_ 
lvcreate -l2 -m1 --mirrorlog core -n $lv1 $vg $dev1 $dev2 
mimages_are_redundant_ $vg $lv1 
check_and_cleanup_lvs_

#COMM "create 3-way mirror with disklog from 4 PVs"
prepare_lvs_ 
lvcreate -l2 -m2 -n $lv1 $vg $dev1 $dev2 $dev4 $dev3:0-1 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

#COMM "lvcreate --nosync is in 100% sync after creation (bz429342)"
prepare_lvs_ 
lvcreate -l2 -m1 --nosync -n $lv1 $vg $dev1 $dev2 $dev3:0-1 2>out
grep "New mirror won't be synchronised." out
lvs -o copy_percent --noheadings $vg/$lv1 |grep 100.00
check_and_cleanup_lvs_

# ---
# convert

#COMM "convert from linear to 2-way mirror"
prepare_lvs_ 
lvcreate -l2 -n $lv1 $vg $dev1 
lvconvert -m+1 $vg/$lv1 $dev2 $dev3:0-1 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

#COMM "convert from 2-way mirror to linear"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
lvconvert -m-1 $vg/$lv1 
mimages_are_redundant_ $vg $lv1 
check_and_cleanup_lvs_

for status in active inactive; do 
# bz192865 lvconvert log of an inactive mirror lv
#COMM "convert from disklog to corelog"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
	test $status = "inactive" && lvchange -an $vg/$lv1
	yes | lvconvert --mirrorlog core $vg/$lv1 
mimages_are_redundant_ $vg $lv1 
check_and_cleanup_lvs_

#COMM "convert from corelog to disklog"
prepare_lvs_ 
lvcreate -l2 -m1 --mirrorlog core -n $lv1 $vg $dev1 $dev2 
	test $status = "inactive" && lvchange -an $vg/$lv1
lvconvert --mirrorlog disk $vg/$lv1 $dev3:0-1 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_
done

# ---
# resize

#COMM "extend 2-way mirror"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
lvchange -an $vg/$lv1 
lvextend -l+2 $vg/$lv1 
mimages_are_redundant_ $vg $lv1 
mimages_are_contiguous_ $vg $lv1 
check_and_cleanup_lvs_

#COMM "reduce 2-way mirror"
prepare_lvs_ 
lvcreate -l4 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
lvchange -an $vg/$lv1 
lvreduce -l-2 $vg/$lv1 
check_and_cleanup_lvs_

#COMM "extend 2-way mirror (cling if not contiguous)"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
lvcreate -l1 -n $lv2 $vg $dev1 
lvcreate -l1 -n $lv3 $vg $dev2 
lvchange -an $vg/$lv1 
lvextend -l+2 $vg/$lv1 
mimages_are_redundant_ $vg $lv1 
mimages_are_clung_ $vg $lv1 
check_and_cleanup_lvs_

# ---
# failure cases

#COMM "create 2-way mirror with disklog from 2 PVs"
prepare_lvs_ 
not lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2
# "(cleanup previous test)"
check_and_cleanup_lvs_

#COMM "convert linear to 2-way mirror with 1 PV"
prepare_lvs_ 
lvcreate -l2 -n $lv1 $vg $dev1 
not lvconvert -m+1 --mirrorlog core $vg/$lv1 $dev1
# "(cleanup previous test)"
check_and_cleanup_lvs_

# ---
# resync
# FIXME: using dm-delay to properly check whether the resync really started

#COMM "force resync 2-way active mirror"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
yes | lvchange --resync $vg/$lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

#COMM "force resync 2-way inactive mirror"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
lvchange -an $vg/$lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
lvchange --resync $vg/$lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

# ---------------------------------------------------------------------

