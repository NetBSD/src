# Copyright (C) 2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

test_description="foo" # silence test-lib for now
. ./test-lib.sh

aux() {
        # use just "$@" for verbose operation
	"$@" > /dev/null 2> /dev/null
	#"$@"
}

STACKTRACE() {
	trap - ERR;
	i=0;
	while FUNC=${FUNCNAME[$i]}; test "$FUNC" != "main"; do 
		echo "$i ${FUNC}() called from ${BASH_SOURCE[$i]}:${BASH_LINENO[$i]}"
		i=$(($i + 1));
	done
}	

teardown() {
	echo $LOOP
	echo $PREFIX

	test -n "$PREFIX" && {
		rm -rf $G_root_/dev/$PREFIX*
		while dmsetup table | grep -q ^$PREFIX; do
			for s in `dmsetup table | grep ^$PREFIX| awk '{ print substr($1,1,length($1)-1) }'`; do
				dmsetup resume $s 2>/dev/null > /dev/null || true
				dmsetup remove $s 2>/dev/null > /dev/null || true
			done
		done
	}

	test -n "$LOOP" && losetup -d $LOOP
	test -n "$LOOPFILE" && rm -f $LOOPFILE
}

teardown_() {
	teardown
	cleanup_ # user-overridable cleanup
	testlib_cleanup_ # call test-lib cleanup routine, too
}

make_ioerror() {
	echo 0 10000000 error | dmsetup create ioerror
	dmsetup resume ioerror
	ln -s $G_dev_/mapper/ioerror $G_dev_/ioerror
}

prepare_loop() {
	size=$1
	test -n "$size" || size=32

	test -n "$LOOP" && return 0
	trap 'aux teardown_' EXIT # don't forget to clean up
	trap 'set +vex; STACKTRACE; set -vex' ERR
	#trap - ERR

	LOOPFILE="$PWD/test.img"
	dd if=/dev/zero of="$LOOPFILE" bs=$((1024*1024)) count=1 seek=$(($size-1))
	if LOOP=`losetup -s -f "$LOOPFILE" 2>/dev/null`; then
		return 0
	elif LOOP=`losetup -f` && losetup $LOOP "$LOOPFILE"; then
		# no -s support
		return 0
	else
		# no -f support 
		# Iterate through $G_dev_/loop{,/}{0,1,2,3,4,5,6,7}
		for slash in '' /; do
			for i in 0 1 2 3 4 5 6 7; do
				local dev=$G_dev_/loop$slash$i
				! losetup $dev >/dev/null 2>&1 || continue
				# got a free
				losetup "$dev" "$LOOPFILE"
				LOOP=$dev
				break
			done
			if [ -n "$LOOP" ]; then 
				break
			fi
		done
		test -n "$LOOP" # confirm or fail
		return 0
	fi
	exit 1 # should not happen
}

prepare_devs() {
	local n="$1"
	test -z "$n" && n=3
	local devsize="$2"
	test -z "$devsize" && devsize=33
	local pvname="$3"
	test -z "$pvname" && pvname="pv"

	prepare_loop $(($n*$devsize))

	PREFIX="LVMTEST$$"

	if ! loopsz=`blockdev --getsz $LOOP 2>/dev/null`; then
  		loopsz=`blockdev --getsize $LOOP 2>/dev/null`
	fi

	local size=$(($loopsz/$n))

	for i in `seq 1 $n`; do
		local name="${PREFIX}$pvname$i"
		local dev="$G_dev_/mapper/$name"
		eval "dev$i=$dev"
		devs="$devs $dev"
		echo 0 $size linear $LOOP $((($i-1)*$size)) > $name.table
		dmsetup create $name $name.table
		dmsetup resume $name
	done

    # set up some default names
	vg=${PREFIX}vg
	vg1=${PREFIX}vg1
	vg2=${PREFIX}vg2
	lv=LV
	lv1=LV1
	lv2=LV2
	lv3=LV3
	lv4=LV4
}

disable_dev() {
	for dev in "$@"; do
        # first we make the device inaccessible
		echo 0 10000000 error | dmsetup load $dev
		dmsetup resume $dev
        # now let's try to get rid of it if it's unused
        #dmsetup remove $dev
	done
}

enable_dev() {
	for dev in "$@"; do
		local name=`echo "$dev" | sed -e 's,.*/,,'`
		dmsetup create $name $name.table || dmsetup load $name $name.table
		dmsetup resume $dev
	done
}

backup_dev() {
	for dev in "$@"; do
		dd if=$dev of=$dev.backup bs=1024
	done
}

restore_dev() {
	for dev in "$@"; do
		test -e $dev.backup || {
			echo "Internal error: $dev not backed up, can't restore!"
			exit 1
		}
		dd of=$dev if=$dev.backup bs=1024
	done
}

prepare_pvs() {
	prepare_devs "$@"
	pvcreate $devs
}

prepare_vg() {
	prepare_pvs "$@"
	vgcreate $vg $devs
}

prepare_lvmconf() {
	local filter="$1"
	test -z "$filter" && \
		filter='[ "a/dev\/mirror/", "a/dev\/mapper\/.*pv[0-9_]*$/", "r/.*/" ]'
	cat > $G_root_/etc/lvm.conf <<-EOF
  devices {
    dir = "$G_dev_"
    scan = "$G_dev_"
    filter = $filter
    cache_dir = "$G_root_/etc"
    sysfs_scan = 0
  }
  log {
    verbose = $verboselevel
    syslog = 0
    indent = 1
  }
  backup {
    backup = 0
    archive = 0
  }
  global {
    library_dir = "$G_root_/lib"
     locking_dir = "$G_root_/var/lock/lvm"
  }
EOF
}

set -vexE
aux prepare_lvmconf

