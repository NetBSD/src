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

#
# tests basic functionality of read-ahead and ra regressions
#

test_description='Test read-ahead functionality'

. ./test-utils.sh


get_lvs_() {
   lvs --units s --nosuffix --noheadings -o $1 "$vg"/"$lv"
}

check_lvs_() {
   case $(get_lvs_ $1) in
    *$2) true ;;
    *) false ;;
   esac
}

aux prepare_vg 5

#COMM "test various read ahead settings (bz450922)"
lvcreate -n "$lv" -l 100%FREE -i5 -I256 "$vg"
ra="$(get_lvs_ lv_kernel_read_ahead)"
test "$(( ( $ra / 5 ) * 5 ))" -eq $ra
lvdisplay "$vg"/"$lv"
lvchange -r auto "$vg"/"$lv" 2>&1 | grep auto
check_lvs_ lv_read_ahead auto
check_lvs_ lv_kernel_read_ahead 5120
lvchange -r 640 "$vg/$lv"
check_lvs_ lv_read_ahead 640
lvremove -ff "$vg"

#COMM "read ahead is properly inherited from underlying PV"
blockdev --setra 768 $dev1
lvcreate -n $lv -L4m $vg $dev1
test $(blockdev --getra $G_dev_/$vg/$lv) -eq 768
lvremove -ff $vg

# Check default, active/inactive values for read_ahead / kernel_read_ahead
lvcreate -n $lv -l 50%FREE $vg
lvchange -an $vg/$lv
check_lv_field_ $vg/$lv lv_read_ahead auto
check_lv_field_ $vg/$lv lv_kernel_read_ahead -1
lvchange -r 512 $vg/$lv
lvchange -ay $vg/$lv
check_lv_field_ $vg/$lv lv_read_ahead 256.00k
check_lv_field_ $vg/$lv lv_kernel_read_ahead 256.00k
lvremove -ff $vg
