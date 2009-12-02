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

test_description="ensure that pvmove works with basic options"

. ./test-utils.sh

dmsetup_has_dm_devdir_support_ || exit 200

# ---------------------------------------------------------------------
# Utilities

lvdev_() {
  echo "$G_dev_/$1/$2"
}

lv_is_on_() {
  local lv=$1 #allready vg/lv
  shift 1
  lvs -a -odevices --noheadings $lv | sed 's/,/\n/g' > out
#is on all specified devs
  for d in $*; do grep "$d(" out; done
#isn't on any other dev (we are set -e remember)
  for d in $*; do ! grep -v "$d(" out; done
  return 0
}

save_dev_sum_() {
  mkfs.ext3 $1 > /dev/null &&
  md5sum $1 > md5.$(basename $1)
}

check_dev_sum_() {
  md5sum $1 > md5.tmp && cmp md5.$(basename $1) md5.tmp
}

# ---------------------------------------------------------------------
# Initialize PVs and VGs

aux prepare_vg 5 80

# ---------------------------------------------------------------------
# Common environment setup/cleanup for each sub testcases

prepare_lvs_() {
  lvcreate -l2 -n $lv1 $vg $dev1 
    lv_is_on_ $vg/$lv1 $dev1 
  lvcreate -l9 -i3 -n $lv2 $vg $dev2 $dev3 $dev4 
    lv_is_on_ $vg/$lv2 $dev2 $dev3 $dev4 
  lvextend -l+2 $vg/$lv1 $dev2 
    lv_is_on_ $vg/$lv1 $dev1 $dev2 
  lvextend -l+2 $vg/$lv1 $dev3 
    lv_is_on_ $vg/$lv1 $dev1 $dev2 $dev3 
  lvextend -l+2 $vg/$lv1 $dev1 
    lv_is_on_ $vg/$lv1 $dev1 $dev2 $dev3 $dev1 
  lvcreate -l1 -n $lv3 $vg $dev2 
    lv_is_on_ $vg/$lv3 $dev2 
  save_dev_sum_ $(lvdev_ $vg $lv1) 
  save_dev_sum_ $(lvdev_ $vg $lv2) 
  save_dev_sum_ $(lvdev_ $vg $lv3) 
  lvs -a -o devices --noheadings $vg/$lv1 > ${lv1}_devs 
  lvs -a -o devices --noheadings $vg/$lv2 > ${lv2}_devs 
  lvs -a -o devices --noheadings $vg/$lv3 > ${lv3}_devs
}

lv_not_changed_() {
  lvs -a -o devices --noheadings $1 > out
  diff $(basename $1)_devs out
}

check_and_cleanup_lvs_() {
  lvs -a -o+devices $vg
  check_dev_sum_ $(lvdev_ $vg $lv1)
  check_dev_sum_ $(lvdev_ $vg $lv2)
  check_dev_sum_ $(lvdev_ $vg $lv3)
  lvs -a -o name $vg > out && ! grep ^pvmove out
  lvremove -ff $vg
	if ! dmsetup table|not grep $vg; then
		echo "ERROR: lvremove did leave some some mappings in DM behind!" &&
		return 1
	fi
	:
}

#COMM "check environment setup/cleanup"
prepare_lvs_
check_and_cleanup_lvs_

# ---------------------------------------------------------------------
# pvmove tests

# ---
# filter by LV

#COMM "only specified LV is moved: from pv2 to pv5 only for lv1"
prepare_lvs_ 
pvmove -i1 -n $vg/$lv1 $dev2 $dev5 
lv_is_on_ $vg/$lv1 $dev1 $dev5 $dev3 $dev1 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

# ---
# segments in a LV

#COMM "the 1st seg of 3-segs LV is moved: from pv1 of lv1 to pv4"
prepare_lvs_ 
pvmove -i1 -n $vg/$lv1 $dev1 $dev4 
lv_is_on_ $vg/$lv1 $dev4 $dev2 $dev3 $dev4 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "the 2nd seg of 3-segs LV is moved: from pv2 of lv1 to pv4"
prepare_lvs_ 
pvmove -i1 -n $vg/$lv1 $dev2 $dev4 
lv_is_on_ $vg/$lv1 $dev1 $dev4 $dev3 $dev1 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "the 3rd seg of 3-segs LV is moved: from pv3 of lv1 to pv4"
prepare_lvs_ 
pvmove -i1 -n $vg/$lv1 $dev3 $dev4 
lv_is_on_ $vg/$lv1 $dev1 $dev2 $dev4 $dev1 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

# ---
# multiple LVs matching

#COMM "1 out of 3 LVs is moved: from pv4 to pv5"
prepare_lvs_ 
pvmove -i1 $dev4 $dev5 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev2 $dev3 $dev5 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "2 out of 3 LVs are moved: from pv3 to pv5"
prepare_lvs_ 
pvmove -i1 $dev3 $dev5 
lv_is_on_ $vg/$lv1 $dev1 $dev2 $dev5 $dev1 
lv_is_on_ $vg/$lv2 $dev2 $dev5 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "3 out of 3 LVs are moved: from pv2 to pv5"
prepare_lvs_ 
pvmove -i1 $dev2 $dev5 
lv_is_on_ $vg/$lv1 $dev1 $dev5 $dev3 $dev1 
lv_is_on_ $vg/$lv2 $dev5 $dev3 $dev4 
lv_is_on_ $vg/$lv3 $dev5 
check_and_cleanup_lvs_

# ---
# areas of striping

#COMM "move the 1st stripe: from pv2 of lv2 to pv1"
prepare_lvs_ 
pvmove -i1 -n $vg/$lv2 $dev2 $dev1 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev1 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "move the 2nd stripe: from pv3 of lv2 to pv1"
prepare_lvs_ 
pvmove -i1 -n $vg/$lv2 $dev3 $dev1 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev2 $dev1 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "move the 3rd stripe: from pv4 of lv2 to pv1"
prepare_lvs_ 
pvmove -i1 -n $vg/$lv2 $dev4 $dev1 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev2 $dev3 $dev1 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

# ---
# partial segment match (source segment splitted)

#COMM "match to the start of segment:from pv2:0-0 to pv5"
prepare_lvs_ 
pvmove -i1 $dev2:0-0 $dev5 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev5 $dev2 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "match to the middle of segment: from pv2:1-1 to pv5"
prepare_lvs_ 
pvmove -i1 $dev2:1-1 $dev5 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev2 $dev5 $dev2 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "match to the end of segment: from pv2:2-2 to pv5"
prepare_lvs_ 
pvmove -i1 $dev2:2-2 $dev5 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev2 $dev5 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

# ---
# destination segment splitted

#COMM "no destination split: from pv2:0-2 to pv5"
prepare_lvs_ 
pvmove -i1 $dev2:0-2 $dev5 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev5 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "destination split into 2: from pv2:0-2 to pv5:5-5 and pv4:5-6"
prepare_lvs_ 
pvmove -i1 $dev2:0-2 $dev5:5-5 $dev4:5-6 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev5 $dev4 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "destination split into 3: from pv2:0-2 to {pv3,4,5}:5-5"
prepare_lvs_ 
pvmove -i1 $dev2:0-2 $dev3:5-5 $dev4:5-5 $dev5:5-5 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev3 $dev4 $dev5 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

# ---
# alloc policy (anywhere, contiguous) with both success and failure cases

#COMM "alloc normal on same PV for source and destination: from pv3:0-2 to pv3:5-7" 
prepare_lvs_ 
not pvmove -i1 $dev3:0-2 $dev3:5-7
# "(cleanup previous test)"
lv_not_changed_ $vg/$lv1 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "alloc anywhere on same PV for source and destination: from pv3:0-2 to pv3:5-7"
prepare_lvs_ 
pvmove -i1 --alloc anywhere $dev3:0-2 $dev3:5-7 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev2 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "alloc anywhere but better area available: from pv3:0-2 to pv3:5-7 or pv5:5-6,pv4:5-5"
prepare_lvs_ 
pvmove -i1 --alloc anywhere $dev3:0-2 $dev3:5-7 $dev5:5-6 $dev4:5-5 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev2 $dev5 $dev4 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "alloc contiguous but area not available: from pv2:0-2 to pv5:5-5 and pv4:5-6"
prepare_lvs_ 
not pvmove -i1 --alloc contiguous $dev2:0-2 $dev5:5-5 $dev4:5-6
# "(cleanup previous test)"
lv_not_changed_ $vg/$lv1 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "alloc contiguous and contiguous area available: from pv2:0-2 to pv5:0-0,pv5:3-5 and pv4:5-6"
prepare_lvs_ 
pvmove -i1 --alloc contiguous $dev2:0-2 $dev5:0-0 $dev5:3-5 $dev4:5-6 
lv_not_changed_ $vg/$lv1 
lv_is_on_ $vg/$lv2 $dev5 $dev3 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

# ---
# multiple segments in a LV

#COMM "multiple source LVs: from pv3 to pv5"
prepare_lvs_ 
pvmove -i1 $dev3 $dev5 
lv_is_on_ $vg/$lv1 $dev1 $dev2 $dev5 
lv_is_on_ $vg/$lv2 $dev2 $dev5 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

# ---
# move inactive LV

#COMM "move inactive LV: from pv2 to pv5"
prepare_lvs_ 
lvchange -an $vg/$lv1 
lvchange -an $vg/$lv3 
pvmove -i1 $dev2 $dev5 
lv_is_on_ $vg/$lv1 $dev1 $dev5 $dev3 
lv_is_on_ $vg/$lv2 $dev5 $dev3 $dev4 
lv_is_on_ $vg/$lv3 $dev5 
check_and_cleanup_lvs_

# ---
# other failure cases

#COMM "no PEs to move: from pv3 to pv1"
prepare_lvs_ 
pvmove -i1 $dev3 $dev1 
not pvmove -i1 $dev3 $dev1
# "(cleanup previous test)"
lv_is_on_ $vg/$lv1 $dev1 $dev2 $dev1 
lv_is_on_ $vg/$lv2 $dev2 $dev1 $dev4 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "no space available: from pv2:0-0 to pv1:0-0" 
prepare_lvs_ 
not pvmove -i1 $dev2:0-0 $dev1:0-0
# "(cleanup previous test)"
lv_not_changed_ $vg/$lv1 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM 'same source and destination: from pv1 to pv1'
prepare_lvs_ 
not pvmove -i1 $dev1 $dev1
#"(cleanup previous test)"
lv_not_changed_ $vg/$lv1 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

#COMM "sum of specified destination PEs is large enough, but it includes source PEs and the free PEs are not enough"
prepare_lvs_ 
not pvmove --alloc anywhere $dev1:0-2 $dev1:0-2 $dev5:0-0 2> err
#"(cleanup previous test)"
grep "Insufficient free space" err 
lv_not_changed_ $vg/$lv1 
lv_not_changed_ $vg/$lv2 
lv_not_changed_ $vg/$lv3 
check_and_cleanup_lvs_

# ---------------------------------------------------------------------

#COMM "pvmove abort"
prepare_lvs_ 
pvmove -i100 -b $dev1 $dev3 
pvmove --abort 
check_and_cleanup_lvs_

#COMM "pvmove out of --metadatacopies 0 PV (bz252150)"
vgremove -ff $vg
pvcreate $devs
pvcreate --metadatacopies 0 $dev1 $dev2
vgcreate $vg $devs
lvcreate -l4 -n $lv1 $vg $dev1
pvmove $dev1

#COMM "pvmove fails activating mirror, properly restores state before pvmove"
dmsetup create "$vg-pvmove0" --notable
not pvmove -i 1 $dev2
test $(dmsetup info --noheadings -c -o suspended "$vg-$lv1") = "Active"
dmsetup remove "$vg-pvmove0"
