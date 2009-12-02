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

. ./test-utils.sh

dmsetup_has_dm_devdir_support_ || exit 200

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

mirrorlog_is_on_()
{
  local lv="$1"_mlog
  shift 1
  lvs -a -odevices --noheadings $lv | sed 's/,/\n/g' > out
  for d in $*; do grep "$d(" out || return 1; done
  for d in $*; do grep -v "$d(" out > out2 || true; mv out2 out; done
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

aux prepare_vg 5

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

# ---------------------------------------------------------------------
# mirrored LV tests

# ---
# add mirror to mirror

# add 1 mirror
prepare_lvs_
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1
check_mirror_count_ $vg/$lv1 2
check_mirror_log_ $vg/$lv1
lvconvert -m+1 -i1 $vg/$lv1 $dev4
check_no_tmplvs_ $vg/$lv1
check_mirror_count_ $vg/$lv1 3
mimages_are_redundant_ $vg $lv1
mirrorlog_is_on_ $vg/$lv1 $dev3
check_and_cleanup_lvs_

# add 2 mirrors
prepare_lvs_
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1
check_mirror_count_ $vg/$lv1 2
check_mirror_log_ $vg/$lv1
lvconvert -m+2 -i1 $vg/$lv1 $dev4 $dev5
check_no_tmplvs_ $vg/$lv1
check_mirror_count_ $vg/$lv1 4
mimages_are_redundant_ $vg $lv1
mirrorlog_is_on_ $vg/$lv1 $dev3
check_and_cleanup_lvs_

# add 1 mirror to core log mirror
prepare_lvs_
lvcreate -l2 -m1 --mirrorlog core -n $lv1 $vg $dev1 $dev2
check_mirror_count_ $vg/$lv1 2
not check_mirror_log_ $vg/$lv1
lvconvert -m+1 -i1 --mirrorlog core $vg/$lv1 $dev4 
check_no_tmplvs_ $vg/$lv1 
check_mirror_count_ $vg/$lv1 3 
not check_mirror_log_ $vg/$lv1
mimages_are_redundant_ $vg $lv1 
check_and_cleanup_lvs_

# add 2 mirrors to core log mirror" 
prepare_lvs_ 
lvcreate -l2 -m1 --mirrorlog core -n $lv1 $vg $dev1 $dev2 
check_mirror_count_ $vg/$lv1 2 
not check_mirror_log_ $vg/$lv1 
lvconvert -m+2 -i1 --mirrorlog core $vg/$lv1 $dev4 $dev5 
check_no_tmplvs_ $vg/$lv1 
check_mirror_count_ $vg/$lv1 4 
not check_mirror_log_ $vg/$lv1
mimages_are_redundant_ $vg $lv1 
check_and_cleanup_lvs_

# ---
# add to converting mirror

# add 1 mirror then add 1 more mirror during conversion
prepare_lvs_
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1
check_mirror_count_ $vg/$lv1 2
check_mirror_log_ $vg/$lv1
lvconvert -m+1 -b $vg/$lv1 $dev4
lvconvert -m+1 -i3 $vg/$lv1 $dev5
check_no_tmplvs_ $vg/$lv1
check_mirror_count_ $vg/$lv1 4
mimages_are_redundant_ $vg $lv1
mirrorlog_is_on_ $vg/$lv1 $dev3
check_and_cleanup_lvs_

# ---
# add mirror and disk log

# "add 1 mirror and disk log" 
prepare_lvs_ 
lvcreate -l2 -m1 --mirrorlog core -n $lv1 $vg $dev1 $dev2 
check_mirror_count_ $vg/$lv1 2 
not check_mirror_log_ $vg/$lv1 
lvconvert -m+1 --mirrorlog disk -i1 $vg/$lv1 $dev4 $dev3:0-1 
check_no_tmplvs_ $vg/$lv1 
check_mirror_count_ $vg/$lv1 3 
check_mirror_log_ $vg/$lv1 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

# ---
# check polldaemon restarts

# convert inactive mirror and start polling
prepare_lvs_
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1
check_mirror_count_ $vg/$lv1 2
lvchange -an $vg/$lv1
lvconvert -m+1 $vg/$lv1 $dev4
lvchange -ay $vg/$lv1
wait_conversion_ $vg/$lv1
lvs -a
check_no_tmplvs_ $vg/$lv1
check_and_cleanup_lvs_

# ---------------------------------------------------------------------
# removal during conversion

# "remove newly added mirror" 
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
check_mirror_count_ $vg/$lv1 2 
check_mirror_log_ $vg/$lv1 
lvconvert -m+1 -b $vg/$lv1 $dev4 
lvconvert -m-1 $vg/$lv1 $dev4 
wait_conversion_ $vg/$lv1 
check_no_tmplvs_ $vg/$lv1 
check_mirror_count_ $vg/$lv1 2 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

# "remove one of newly added mirrors" 
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
check_mirror_count_ $vg/$lv1 2 
check_mirror_log_ $vg/$lv1 
lvconvert -m+2 -b $vg/$lv1 $dev4 $dev5 
lvconvert -m-1 $vg/$lv1 $dev4 
lvconvert -i1 $vg/$lv1 
wait_conversion_ $vg/$lv1 
check_no_tmplvs_ $vg/$lv1 
check_mirror_count_ $vg/$lv1 3 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

# "remove from original mirror (the original is still mirror)"
prepare_lvs_ 
lvcreate -l2 -m2 -n $lv1 $vg $dev1 $dev2 $dev5 $dev3:0-1 
check_mirror_count_ $vg/$lv1 3 
check_mirror_log_ $vg/$lv1 
lvconvert -m+1 -b $vg/$lv1 $dev4 
lvconvert -m-1 $vg/$lv1 $dev2 
lvconvert -i1 $vg/$lv1 
wait_conversion_ $vg/$lv1 
check_no_tmplvs_ $vg/$lv1 
check_mirror_count_ $vg/$lv1 3 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

# "remove from original mirror (the original becomes linear)"
prepare_lvs_ 
lvcreate -l2 -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0-1 
check_mirror_count_ $vg/$lv1 2 
check_mirror_log_ $vg/$lv1 
lvconvert -m+1 -b $vg/$lv1 $dev4 
lvconvert -m-1 $vg/$lv1 $dev2 
lvconvert -i1 $vg/$lv1 
wait_conversion_ $vg/$lv1 
check_no_tmplvs_ $vg/$lv1 
check_mirror_count_ $vg/$lv1 2 
mimages_are_redundant_ $vg $lv1 
mirrorlog_is_on_ $vg/$lv1 $dev3 
check_and_cleanup_lvs_

# ---------------------------------------------------------------------

# "rhbz440405: lvconvert -m0 incorrectly fails if all PEs allocated"
prepare_lvs_ 
lvcreate -l`pvs --noheadings -ope_count $dev1` -m1 -n $lv1 $vg $dev1 $dev2 $dev3:0 
check_mirror_count_ $vg/$lv1 2 
check_mirror_log_ $vg/$lv1 
lvconvert -m0 $vg/$lv1 $dev1 
check_no_tmplvs_ $vg/$lv1 
check_mirror_count_ $vg/$lv1 1 
check_and_cleanup_lvs_

# "rhbz264241: lvm mirror doesn't lose it's "M" --nosync attribute after being down and the up converted"
prepare_lvs_
lvcreate -l2 -m1 -n$lv1 --nosync $vg 
lvconvert -m0 $vg/$lv1
lvconvert -m1 $vg/$lv1
lvs --noheadings -o attr $vg/$lv1 | grep '^ *m'
check_and_cleanup_lvs_

# lvconvert from linear (on multiple PVs) to mirror
prepare_lvs_
lvcreate -l 8 -n $lv1 $vg $dev1:0-3 $dev2:0-3
lvconvert -m1 $vg/$lv1
check_mirror_count_ $vg/$lv1 2
check_mirror_log_ $vg/$lv1
check_and_cleanup_lvs_

# BZ 463272: disk log mirror convert option is lost if downconvert option is also given
prepare_lvs_
lvcreate -l1 -m2 --corelog -n $lv1 $vg
lvconvert -m1 --mirrorlog disk $vg/$lv1
check_mirror_log_ $vg/$lv1
