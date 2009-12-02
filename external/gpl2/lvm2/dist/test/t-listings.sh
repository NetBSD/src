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
# tests functionality of lvs, pvs, vgs, *display tools
#

. ./test-utils.sh

get_lvs_()
{
  case $(lvs --units s --nosuffix --noheadings -o $1_read_ahead "$vg"/"$lv") in
    *$2) true ;;
    *) false ;;
  esac
}

aux prepare_devs 5

pvcreate $dev1
pvcreate --metadatacopies 0 $dev2
pvcreate --metadatacopies 0 $dev3
pvcreate $dev4
pvcreate --metadatacopies 0 $dev5

#COMM bz195276 -- pvs doesn't show PVs until a VG is created
pvs --noheadings|tee out
test $(wc -l <out) -eq 5

#COMM pvs with segment attributes works even for orphans
pvs --noheadings -o  seg_all,pv_all,lv_all,vg_all | tee out
test $(wc -l <out) -eq 5

vgcreate -c n $vg $devs

#COMM pvs and vgs report mda_count, mda_free (bz202886, bz247444)
pvs -o +pv_mda_count,pv_mda_free $devs
for I in $dev2 $dev3 $dev5; do
	aux check_pv_field_ $I pv_mda_count 0
	aux check_pv_field_ $I pv_mda_free 0
done
vgs -o +vg_mda_count,vg_mda_free $vg
aux check_vg_field_ $vg vg_mda_count 2

#COMM pvs doesn't display --metadatacopies 0 PVs as orphans (bz409061)
pvdisplay $dev2|grep "VG Name.*$vg"
test $(pvs -o vg_name --noheadings $dev2) = $vg

#COMM lvs displays snapshots (bz171215)
lvcreate -l4 -n $lv1 $vg
lvcreate -l4 -s -n $lv2 $vg/$lv1
lvs $vg --noheadings|tee out
test $(wc -l <out) -eq 2
lvs -a --noheadings|tee out
# should lvs -a display cow && real devices? (it doesn't)
test $(wc -l <out) -eq 2
dmsetup ls|grep $PREFIX|grep -v "LVMTEST.*pv."
lvremove -f $vg/$lv2

#COMM lvs -a displays mirror legs and log
lvcreate -l4  -m2 -n$lv3 $vg
lvs $vg --noheadings|tee out
test $(wc -l <out) -eq 2
lvs -a --noheadings|tee out
test $(wc -l <out) -eq 6
dmsetup ls|grep $PREFIX|grep -v "LVMTEST.*pv."

#COMM vgs with options from pvs still treats arguments as VGs (bz193543)
vgs -o pv_name,vg_name $vg
# would complain if not

#COMM pvdisplay --maps feature (bz149814)
pvdisplay $devs >out
pvdisplay --maps $devs >out2
not diff out out2

