# Copyright (C) 2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

. ./test-utils.sh

dmsetup_has_dm_devdir_support_ || exit 200

aux prepare_vg 5


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

# ---------------------------------------------------------------------
# Common environment setup/cleanup for each sub testcases

prepare_lvs_()
{
	lvremove -ff $vg
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

prepare_lvs_
check_and_cleanup_lvs_


#COMM "convert from 2-way mirror to linear -- specify leg to remove (bz453643)"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
lvconvert -m0 $vg/$lv1 $dev2
lv_is_on_ $vg/$lv1 $dev1
check_and_cleanup_lvs_

prepare_lvs_ 
lvcreate -l2 -m2 -n $lv1 $vg $dev1 $dev2 $dev4 $dev3:0-1 
lvconvert -m-1 $vg/$lv1 $dev1
lvs -a -o+devices 
mimages_are_on_ $lv1 $dev2 $dev4 
lvconvert -m-1 $vg/$lv1 $dev2
lvs -a -o+devices 
lv_is_on_ $vg/$lv1 $dev4 
check_and_cleanup_lvs_

