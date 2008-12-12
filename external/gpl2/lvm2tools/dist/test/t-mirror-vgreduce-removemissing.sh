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

test_description="ensure that 'vgreduce --removemissing' works on mirrored LV"

. ./test-utils.sh

dmsetup_has_dm_devdir_support_ || exit 200

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
	local pvs="$*"
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
	lvs -o stripes,attr --noheadings $vg/$1 | sed 's/ //g'
	lvs -o stripes,attr --noheadings $vg/$1 | sed 's/ //g' | grep -q '^1-'
}

rest_pvs_()
{
	local index=$1
	local num=$2
	local rem=""
	local n

	for n in $(seq 1 $(($index - 1))) $(seq $(($index + 1)) $num); do
		eval local dev=$\dev$n
		rem="$rem $dev"
	done

	echo "$rem"
}

# ---------------------------------------------------------------------
# Initialize PVs and VGs

prepare_vg 5

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

recover_vg_()
{
	enable_dev $* 
	pvcreate -ff $* 
	vgextend $vg $* 
	check_and_cleanup_lvs_
}

#COMM "check environment setup/cleanup" 
prepare_lvs_ 
check_and_cleanup_lvs_

# ---------------------------------------------------------------------
# one of mirror images has failed

#COMM "basic: fail the 2nd mirror image of 2-way mirrored LV"
prepare_lvs_
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1
lvchange -an $vg/$lv1
aux mimages_are_on_ $lv1 $dev1 $dev2
mirrorlog_is_on_ $lv1 $dev3
disable_dev $dev2
vgreduce --removemissing --force $vg
lv_is_linear_ $lv1
lv_is_on_ $lv1 $dev1

# "cleanup"
recover_vg_ $dev2

# ---------------------------------------------------------------------
# LV has 3 images in flat,
# 1 out of 3 images fails

#COMM test_3way_mirror_fail_1_ <PV# to fail>
test_3way_mirror_fail_1_()
{
	local index=$1

	lvcreate -l2 -m2 -n $lv1 $vg $dev1 $dev2 $dev3 $dev4:0-1
	lvchange -an $vg/$lv1
	aux mimages_are_on_ $lv1 $dev1 $dev2 $dev3
	mirrorlog_is_on_ $lv1 $dev4
	eval disable_dev \$dev$index
	vgreduce --removemissing --force $vg
	lvs -a -o+devices $vg
	mimages_are_on_ $lv1 $(rest_pvs_ $index 3)
	mirrorlog_is_on_ $lv1 $dev4
}

for n in $(seq 1 3); do
	#COMM fail mirror image $(($n - 1)) of 3-way mirrored LV"
	prepare_lvs_
	test_3way_mirror_fail_1_ $n
	eval recover_vg_ \$dev$n
done

# ---------------------------------------------------------------------
# LV has 3 images in flat,
# 2 out of 3 images fail

#COMM test_3way_mirror_fail_2_ <PV# NOT to fail>
test_3way_mirror_fail_2_()
{
	local index=$1

	lvcreate -l2 -m2 -n $lv1 $vg $dev1 $dev2 $dev3 $dev4:0-1
	lvchange -an $vg/$lv1
	mimages_are_on_ $lv1 $dev1 $dev2 $dev3
	mirrorlog_is_on_ $lv1 $dev4
	rest_pvs_ $index 3
	disable_dev $(rest_pvs_ $index 3)
	vgreduce --force --removemissing $vg
	lvs -a -o+devices $vg
	aux lv_is_linear_ $lv1
	eval lv_is_on_ $lv1 \$dev$n
}

for n in $(seq 1 3); do
	#COMM fail mirror images other than mirror image $(($n - 1)) of 3-way mirrored LV
	prepare_lvs_
	test_3way_mirror_fail_2_ $n
	recover_vg_ $(rest_pvs_ $n 3)
done

# ---------------------------------------------------------------------
# LV has 4 images, 1 of them is in the temporary mirror for syncing.
# 1 out of 4 images fails

#COMM test_3way_mirror_plus_1_fail_1_ <PV# to fail>
test_3way_mirror_plus_1_fail_1_()
{
	local index=$1

	lvcreate -l2 -m2 -n $lv1 $vg $dev1 $dev2 $dev3 $dev5:0-1 
	lvchange -an $vg/$lv1 
	lvconvert -m+1 $vg/$lv1 $dev4 
	mimages_are_on_ $lv1 $dev1 $dev2 $dev3 $dev4 
	mirrorlog_is_on_ $lv1 $dev5 
	eval disable_dev \$dev$n 
	vgreduce --removemissing --force $vg 
	lvs -a -o+devices $vg 
	mimages_are_on_ $lv1 $(rest_pvs_ $index 4) 
	mirrorlog_is_on_ $lv1 $dev5
}

for n in $(seq 1 4); do
	#COMM "fail mirror image $(($n - 1)) of 4-way (1 converting) mirrored LV"
	prepare_lvs_
	test_3way_mirror_plus_1_fail_1_ $n
	eval recover_vg_ \$dev$n
done

# ---------------------------------------------------------------------
# LV has 4 images, 1 of them is in the temporary mirror for syncing.
# 3 out of 4 images fail

#COMM test_3way_mirror_plus_1_fail_3_ <PV# NOT to fail>
test_3way_mirror_plus_1_fail_3_()
{
	local index=$1

	lvcreate -l2 -m2 -n $lv1 $vg $dev1 $dev2 $dev3 $dev5:0-1 
	lvchange -an $vg/$lv1 
	lvconvert -m+1 $vg/$lv1 $dev4 
	mimages_are_on_ $lv1 $dev1 $dev2 $dev3 $dev4 
	mirrorlog_is_on_ $lv1 $dev5 
	disable_dev $(rest_pvs_ $index 4) 
	vgreduce --removemissing --force $vg 
	lvs -a -o+devices $vg 
	eval local dev=\$dev$n
	mimages_are_on_ $lv1 $dev || lv_is_on_ $lv1 $dev
	not mirrorlog_is_on_ $lv1 $dev5
}

for n in $(seq 1 4); do
	#COMM "fail mirror images other than mirror image $(($n - 1)) of 4-way (1 converting) mirrored LV"
	prepare_lvs_
	test_3way_mirror_plus_1_fail_3_ $n
	recover_vg_ $(rest_pvs_ $n 4)
done

# ---------------------------------------------------------------------
# LV has 4 images, 2 of them are in the temporary mirror for syncing.
# 1 out of 4 images fail

# test_2way_mirror_plus_2_fail_1_ <PV# to fail>
test_2way_mirror_plus_2_fail_1_()
{
	local index=$1

	lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
	lvchange -an $vg/$lv1 
	lvconvert -m+2 $vg/$lv1 $dev3 $dev4 
	mimages_are_on_ $lv1 $dev1 $dev2 $dev3 $dev4 
	mirrorlog_is_on_ $lv1 $dev5 
	eval disable_dev \$dev$n 
	vgreduce --removemissing --force $vg 
	lvs -a -o+devices $vg 
	mimages_are_on_ $lv1 $(rest_pvs_ $index 4) 
	mirrorlog_is_on_ $lv1 $dev5
}

for n in $(seq 1 4); do
	#COMM "fail mirror image $(($n - 1)) of 4-way (2 converting) mirrored LV" 
	prepare_lvs_ 
	test_2way_mirror_plus_2_fail_1_ $n
	eval recover_vg_ \$dev$n
done

# ---------------------------------------------------------------------
# LV has 4 images, 2 of them are in the temporary mirror for syncing.
# 3 out of 4 images fail

# test_2way_mirror_plus_2_fail_3_ <PV# NOT to fail>
test_2way_mirror_plus_2_fail_3_()
{
	local index=$1

	lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
	lvchange -an $vg/$lv1 
	lvconvert -m+2 $vg/$lv1 $dev3 $dev4 
	mimages_are_on_ $lv1 $dev1 $dev2 $dev3 $dev4 
	mirrorlog_is_on_ $lv1 $dev5 
	disable_dev $(rest_pvs_ $index 4) 
	vgreduce --removemissing --force $vg 
	lvs -a -o+devices $vg 
	eval local dev=\$dev$n
	mimages_are_on_ $lv1 $dev || lv_is_on_ $lv1 $dev
	not mirrorlog_is_on_ $lv1 $dev5
}

for n in $(seq 1 4); do
	#COMM "fail mirror images other than mirror image $(($n - 1)) of 4-way (2 converting) mirrored LV"
	prepare_lvs_
	test_2way_mirror_plus_2_fail_3_ $n
	recover_vg_ $(rest_pvs_ $n 4)
done

# ---------------------------------------------------------------------
# log device is gone (flat mirror and stacked mirror)

#COMM "fail mirror log of 2-way mirrored LV" 
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
lvchange -an $vg/$lv1 
mimages_are_on_ $lv1 $dev1 $dev2 
mirrorlog_is_on_ $lv1 $dev5 
disable_dev $dev5 
vgreduce --removemissing --force $vg 
mimages_are_on_ $lv1 $dev1 $dev2 
not mirrorlog_is_on_ $lv1 $dev5
recover_vg_ $dev5

#COMM "fail mirror log of 3-way (1 converting) mirrored LV" 
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
lvchange -an $vg/$lv1 
lvconvert -m+1 $vg/$lv1 $dev3 
mimages_are_on_ $lv1 $dev1 $dev2 $dev3 
mirrorlog_is_on_ $lv1 $dev5 
disable_dev $dev5 
vgreduce --removemissing --force $vg 
mimages_are_on_ $lv1 $dev1 $dev2 $dev3 
not mirrorlog_is_on_ $lv1 $dev5
recover_vg_ $dev5

# ---------------------------------------------------------------------
# all images are gone (flat mirror and stacked mirror)

#COMM "fail all mirror images of 2-way mirrored LV"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
lvchange -an $vg/$lv1 
mimages_are_on_ $lv1 $dev1 $dev2 
mirrorlog_is_on_ $lv1 $dev5 
disable_dev $dev1 $dev2 
vgreduce --removemissing --force $vg 
not lvs $vg/$lv1
recover_vg_ $dev1 $dev2

#COMM "fail all mirror images of 3-way (1 converting) mirrored LV"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
lvchange -an $vg/$lv1 
lvconvert -m+1 $vg/$lv1 $dev3 
mimages_are_on_ $lv1 $dev1 $dev2 $dev3 
mirrorlog_is_on_ $lv1 $dev5 
disable_dev $dev1 $dev2 $dev3 
vgreduce --removemissing --force $vg 
not lvs $vg/$lv1
recover_vg_ $dev1 $dev2 $dev3

# ---------------------------------------------------------------------
# Multiple LVs

#COMM "fail a mirror image of one of mirrored LV"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
lvchange -an $vg/$lv1 
lvcreate -l2 -m1 -n $lv2 $vg $dev3 $dev4 $dev5:1-1 
lvchange -an $vg/$lv2 
mimages_are_on_ $lv1 $dev1 $dev2 
mimages_are_on_ $lv2 $dev3 $dev4 
mirrorlog_is_on_ $lv1 $dev5 
mirrorlog_is_on_ $lv2 $dev5 
disable_dev $dev2 
vgreduce --removemissing --force $vg 
mimages_are_on_ $lv2 $dev3 $dev4 
mirrorlog_is_on_ $lv2 $dev5 
lv_is_linear_ $lv1 
lv_is_on_ $lv1 $dev1
recover_vg_ $dev2

#COMM "fail mirror images, one for each mirrored LV"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
lvchange -an $vg/$lv1 
lvcreate -l2 -m1 -n $lv2 $vg $dev3 $dev4 $dev5:1-1 
lvchange -an $vg/$lv2 
mimages_are_on_ $lv1 $dev1 $dev2 
mimages_are_on_ $lv2 $dev3 $dev4 
mirrorlog_is_on_ $lv1 $dev5 
mirrorlog_is_on_ $lv2 $dev5 
disable_dev $dev2 
disable_dev $dev4 
vgreduce --removemissing --force $vg 
lv_is_linear_ $lv1 
lv_is_on_ $lv1 $dev1 
lv_is_linear_ $lv2 
lv_is_on_ $lv2 $dev3
recover_vg_ $dev2 $dev4

# ---------------------------------------------------------------------
# no failure

#COMM "no failures"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev5:0-1 
lvchange -an $vg/$lv1 
mimages_are_on_ $lv1 $dev1 $dev2 
mirrorlog_is_on_ $lv1 $dev5 
vgreduce --removemissing --force $vg 
mimages_are_on_ $lv1 $dev1 $dev2 
mirrorlog_is_on_ $lv1 $dev5
check_and_cleanup_lvs_

# ---------------------------------------------------------------------

