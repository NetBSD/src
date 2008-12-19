#!/bin/sh
#
# Copyright (C) 2007 Red Hat, Inc. All rights reserved.
#
# This file is part of LVM2.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Zdenek Kabelac <zkabelac at redhat.com>
#
# Script for resizing devices (usable for LVM resize)
#
# Needed utilities:
#   mount, umount, grep, readlink, blockdev, blkid, fsck, xfs_check
#
# ext2/ext3: resize2fs, tune2fs
# reiserfs: resize_reiserfs, reiserfstune
# xfs: xfs_growfs, xfs_info
#

TOOL=fsadm

PATH=/sbin:/usr/sbin:/bin:/usr/sbin:$PATH

# utilities
TUNE_EXT=tune2fs
RESIZE_EXT=resize2fs
TUNE_REISER=reiserfstune
RESIZE_REISER=resize_reiserfs
TUNE_XFS=xfs_info
RESIZE_XFS=xfs_growfs

MOUNT=mount
UMOUNT=umount
MKDIR=mkdir
RMDIR=rmdir
BLOCKDEV=blockdev
BLKID=blkid
GREP=grep
CUT=cut
READLINK=readlink
READLINK_E="-e"
FSCK=fsck
XFS_CHECK=xfs_check

LVRESIZE=lvresize

YES=
DRY=0
VERB=
FORCE=
EXTOFF=0
DO_LVRESIZE=0
FSTYPE=unknown
VOLUME=unknown
TEMPDIR="${TMPDIR:-/tmp}/${TOOL}_${RANDOM}$$/m"
BLOCKSIZE=
BLOCKCOUNT=
MOUNTPOINT=
MOUNTED=
REMOUNT=

IFS_OLD=$IFS

tool_usage() {
	echo "${TOOL}: Utility to resize or check the filesystem on a device"
	echo
	echo "  ${TOOL} [options] check device"
	echo "    - Check the filesystem on device using fsck"
	echo
	echo "  ${TOOL} [options] resize device [new_size[BKMGTPE]]"
	echo "    - Change the size of the filesystem on device to new_size"
	echo
	echo "  Options:"
	echo "    -h | --help         Show this help message"
	echo "    -v | --verbose      Be verbose"
	echo "    -e | --ext-offline  unmount filesystem before Ext2/3 resize"
	echo "    -f | --force        Bypass sanity checks"
	echo "    -n | --dry-run      Print commands without running them"
	echo "    -l | --lvresize     Resize given device (if it is LVM device)"
	echo "    -y | --yes          Answer \"yes\" at any prompts"
	echo
	echo "  new_size - Absolute number of filesystem blocks to be in the filesystem,"
	echo "             or an absolute size using a suffix (in powers of 1024)."
	echo "             If new_size is not supplied, the whole device is used."

	exit
}

verbose() {
	test -n "$VERB" && echo "$TOOL: $@" || true
}

error() {
	echo "$TOOL: $@" >&2
	cleanup 1
}

dry() {
	if [ "$DRY" -ne 0 ]; then
		verbose "Dry execution $@"
		return 0
	fi
	verbose "Executing $@"
	$@
}

cleanup() {
	trap '' 2
	# reset MOUNTPOINT - avoid recursion
	test "$MOUNTPOINT" = "$TEMPDIR" && MOUNTPOINT="" temp_umount
	if [ -n "$REMOUNT" ]; then
		verbose "Remounting unmounted filesystem back"
		dry $MOUNT "$VOLUME" "$MOUNTED"
	fi
	IFS=$IFS_OLD
	trap 2

	# start LVRESIZE with the filesystem modification flag
	# and allow recursive call of fsadm
	unset FSADM_RUNNING
	test "$DO_LVRESIZE" -eq 2 && exec $LVRESIZE $VERB -r -L$(( $NEWSIZE / 1048576 )) $VOLUME
	exit ${1:-0}
}

# convert parameter from Exa/Peta/Tera/Giga/Mega/Kilo/Bytes and blocks
# (2^(60/50/40/30/20/10/0))
decode_size() {
	case "$1" in
	 *[eE]) NEWSIZE=$(( ${1%[eE]} * 1152921504606846976 )) ;;
	 *[pP]) NEWSIZE=$(( ${1%[pP]} * 1125899906842624 )) ;;
	 *[tT]) NEWSIZE=$(( ${1%[tT]} * 1099511627776 )) ;;
	 *[gG]) NEWSIZE=$(( ${1%[gG]} * 1073741824 )) ;;
	 *[mM]) NEWSIZE=$(( ${1%[mM]} * 1048576 )) ;;
	 *[kK]) NEWSIZE=$(( ${1%[kK]} * 1024 )) ;;
	 *[bB]) NEWSIZE=${1%[bB]} ;;
	     *) NEWSIZE=$(( $1 * $2 )) ;;
	esac
	#NEWBLOCKCOUNT=$(round_block_size $NEWSIZE $2)
	NEWBLOCKCOUNT=$(( $NEWSIZE / $2 ))

	if [ $DO_LVRESIZE -eq 1 ]; then
		# start lvresize, but first cleanup mounted dirs
		DO_LVRESIZE=2
		cleanup 0
	fi
}

# detect filesystem on the given device
# dereference device name if it is symbolic link
detect_fs() {
        VOLUME=${1#/dev/}
	VOLUME=$($READLINK $READLINK_E -n "/dev/$VOLUME") || error "Cannot get readlink $1"
	# use /dev/null as cache file to be sure about the result
	# use 'cut' to be compatible with older version of blkid that does not provide option '-o value'
	FSTYPE=$($BLKID -c /dev/null -s TYPE "$VOLUME" | $CUT -d \" -f 2) || error "Cannot get FSTYPE of \"$VOLUME\""
	verbose "\"$FSTYPE\" filesystem found on \"$VOLUME\""
}

# check if the given device is already mounted and where
detect_mounted()  {
	$MOUNT >/dev/null || error "Cannot detect mounted device $VOLUME"
	MOUNTED=$($MOUNT | $GREP "$VOLUME")
	MOUNTED=${MOUNTED##* on }
	MOUNTED=${MOUNTED% type *} # allow type in the mount name
	test -n "$MOUNTED"
}

# get the full size of device in bytes
detect_device_size() {
	# check if blockdev supports getsize64
	$BLOCKDEV 2>&1 | $GREP getsize64 >/dev/null 
	if test $? -eq 0; then 
		DEVSIZE=$($BLOCKDEV --getsize64 "$VOLUME") || error "Cannot read size of device \"$VOLUME\""
	else
		DEVSIZE=$($BLOCKDEV --getsize "$VOLUME") || error "Cannot read size of device \"$VOLUME\""
		SSSIZE=$($BLOCKDEV --getss "$VOLUME") || error "Cannot block size read device \"$VOLUME\""
		DEVSIZE=$(($DEVSIZE * $SSSIZE))
	fi
}

# round up $1 / $2
# could be needed to gaurantee 'at least given size'
# but it makes many troubles
round_up_block_size() {
	echo $(( ($1 + $2 - 1) / $2 ))
}

temp_mount() {
	dry $MKDIR -p -m 0000 "$TEMPDIR" || error "Failed to create $TEMPDIR"
	dry $MOUNT "$VOLUME" "$TEMPDIR" || error "Failed to mount $TEMPDIR"
}

temp_umount() {
	dry $UMOUNT "$TEMPDIR" || error "Failed to umount $TEMPDIR"
	dry $RMDIR "${TEMPDIR}" || error "Failed to remove $TEMPDIR"
	dry $RMDIR "${TEMPDIR%%m}" || error "Failed to remove ${TEMPDIR%%m}"
}

yes_no() {
	echo -n "$@? [Y|n] "
	if [ -n "$YES" ]; then
		ANS="y"; echo -n $ANS
	else
		read -n 1 ANS
	fi
	test -n "$ANS" && echo
	case "$ANS" in
	  "y" | "Y" | "" ) return 0 ;;
	esac
	return 1
}

try_umount() {
	yes_no "Do you want to unmount \"$MOUNTED\"" && dry $UMOUNT "$MOUNTED" && return 0
	error "Cannot proceed test with mounted filesystem \"$MOUNTED\""
}

validate_parsing() {
	test -n "$BLOCKSIZE" -a -n "$BLOCKCOUNT" || error "Cannot parse $1 output"
}
####################################
# Resize ext2/ext3 filesystem
# - unmounted or mounted for upsize
# - unmounted for downsize
####################################
resize_ext() {
	verbose "Parsing $TUNE_EXT -l \"$VOLUME\""
	for i in $($TUNE_EXT -l "$VOLUME"); do
		case "$i" in
		  "Block size"*) BLOCKSIZE=${i##*  } ;;
		  "Block count"*) BLOCKCOUNT=${i##*  } ;;
		esac
	done
	validate_parsing $TUNE_EXT
	decode_size $1 $BLOCKSIZE
	FSFORCE=$FORCE

	if [ "$NEWBLOCKCOUNT" -lt "$BLOCKCOUNT" -o "$EXTOFF" -eq 1 ]; then
		detect_mounted && verbose "$RESIZE_EXT needs unmounted filesystem" && try_umount
		REMOUNT=$MOUNTED
		# CHECKME: after umount resize2fs requires fsck or -f flag.
		FSFORCE="-f"
	fi

	verbose "Resizing \"$VOLUME\" $BLOCKCOUNT -> $NEWBLOCKCOUNT blocks ($NEWSIZE bytes, bs:$BLOCKSIZE)"
	dry $RESIZE_EXT $FSFORCE "$VOLUME" $NEWBLOCKCOUNT
}

#############################
# Resize reiserfs filesystem
# - unmounted for upsize
# - unmounted for downsize
#############################
resize_reiser() {
	detect_mounted
	if [ -n "$MOUNTED" ]; then
		verbose "ReiserFS resizes only unmounted filesystem"
		try_umount
		REMOUNT=$MOUNTED
	fi
	verbose "Parsing $TUNE_REISER \"$VOLUME\""
	for i in $($TUNE_REISER "$VOLUME"); do
		case "$i" in
		  "Blocksize"*) BLOCKSIZE=${i##*: } ;;
		  "Count of blocks"*) BLOCKCOUNT=${i##*: } ;;
		esac
	done
	validate_parsing $TUNE_REISER
	decode_size $1 $BLOCKSIZE
	verbose "Resizing \"$VOLUME\" $BLOCKCOUNT -> $NEWBLOCKCOUNT blocks ($NEWSIZE bytes, bs: $NEWBLOCKCOUNT)"
	if [ -n "$YES" ]; then
		dry echo y | $RESIZE_REISER -s $NEWSIZE "$VOLUME"
	else
		dry $RESIZE_REISER -s $NEWSIZE "$VOLUME"
	fi
}

########################
# Resize XFS filesystem
# - mounted for upsize
# - can not downsize
########################
resize_xfs() {
	detect_mounted
	MOUNTPOINT=$MOUNTED
	if [ -z "$MOUNTED" ]; then
		MOUNTPOINT=$TEMPDIR
		temp_mount || error "Cannot mount Xfs filesystem"
	fi
	verbose "Parsing $TUNE_XFS \"$MOUNTPOINT\""
	for i in $($TUNE_XFS "$MOUNTPOINT"); do
		case "$i" in
		  "data"*) BLOCKSIZE=${i##*bsize=} ; BLOCKCOUNT=${i##*blocks=} ;;
		esac
	done
	BLOCKSIZE=${BLOCKSIZE%%[^0-9]*}
	BLOCKCOUNT=${BLOCKCOUNT%%[^0-9]*}
	validate_parsing $TUNE_XFS
	decode_size $1 $BLOCKSIZE
	if [ $NEWBLOCKCOUNT -gt $BLOCKCOUNT ]; then
		verbose "Resizing Xfs mounted on \"$MOUNTPOINT\" to fill device \"$VOLUME\""
		dry $RESIZE_XFS $MOUNTPOINT
	elif [ $NEWBLOCKCOUNT -eq $BLOCKCOUNT ]; then
		verbose "Xfs filesystem already has the right size"
	else
		error "Xfs filesystem shrinking is unsupported"
	fi
}

####################
# Resize filesystem
####################
resize() {
	NEWSIZE=$2
	detect_fs "$1"
	detect_device_size
	verbose "Device \"$VOLUME\" has $DEVSIZE bytes"
	# if the size parameter is missing use device size
	#if [ -n "$NEWSIZE" -a $NEWSIZE <
	test -z "$NEWSIZE" && NEWSIZE=${DEVSIZE}b
	trap cleanup 2
	#IFS=$'\n'  # don't use bash-ism ??
	IFS="$(printf \"\\n\")"  # needed for parsing output
	case "$FSTYPE" in
	  "ext3"|"ext2") resize_ext $NEWSIZE ;;
	  "reiserfs") resize_reiser $NEWSIZE ;;
	  "xfs") resize_xfs $NEWSIZE ;;
	  *) error "Filesystem \"$FSTYPE\" on device \"$VOLUME\" is not supported by this tool" ;;
	esac || error "Resize $FSTYPE failed"
	cleanup 0
}

###################
# Check filesystem
###################
check() {
	detect_fs "$1"
	case "$FSTYPE" in
	  "xfs") dry $XFS_CHECK "$VOLUME" ;;
	  *) dry $FSCK $YES "$VOLUME" ;;
	esac
}

#############################
# start point of this script
# - parsing parameters
#############################

# test if we are not invoked recursively
test -n "$FSADM_RUNNING" && exit 0

# test some prerequisities
test -n "$TUNE_EXT" -a -n "$RESIZE_EXT" -a -n "$TUNE_REISER" -a -n "$RESIZE_REISER" \
  -a -n "$TUNE_XFS" -a -n "$RESIZE_XFS" -a -n "$MOUNT" -a -n "$UMOUNT" -a -n "$MKDIR" \
  -a -n "$RMDIR" -a -n "$BLOCKDEV" -a -n "$BLKID" -a -n "$GREP" -a -n "$READLINK" \
  -a -n "$FSCK" -a -n "$XFS_CHECK" -a -n "LVRESIZE" -a -n "$CUT" \
  || error "Required command definitions in the script are missing!"

$($READLINK -e -n / >/dev/null 2>&1) || READLINK_E="-f"
TEST64BIT=$(( 1000 * 1000000000000 ))
test $TEST64BIT -eq 1000000000000000 || error "Shell does not handle 64bit arithmetic"
$(echo Y | $GREP Y >/dev/null) || error "Grep does not work properly"


if [ "$1" = "" ] ; then
	tool_usage
fi

while [ "$1" != "" ]
do
	case "$1" in
	 "-h"|"--help") tool_usage ;;
	 "-v"|"--verbose") VERB="-v" ;;
	 "-n"|"--dry-run") DRY=1 ;;
	 "-f"|"--force") FORCE="-f" ;;
	 "-e"|"--ext-offline") EXTOFF=1 ;;
	 "-y"|"--yes") YES="-y" ;;
	 "-l"|"--lvresize") DO_LVRESIZE=1 ;;
	 "check") shift; CHECK=$1 ;;
	 "resize") shift; RESIZE=$1; shift; NEWSIZE=$1 ;;
	 *) error "Wrong argument \"$1\". (see: $TOOL --help)"
	esac
	shift
done

if [ -n "$CHECK" ]; then
	check "$CHECK"
elif [ -n "$RESIZE" ]; then
	export FSADM_RUNNING="fsadm"
	resize "$RESIZE" "$NEWSIZE"
else
	error "Missing command. (see: $TOOL --help)"
fi
#!/bin/sh
#
# Copyright (C) 2007 Red Hat, Inc. All rights reserved.
#
# This file is part of LVM2.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Zdenek Kabelac <zkabelac at redhat.com>
#
# Script for resizing devices (usable for LVM resize)
#
# Needed utilities:
#   mount, umount, grep, readlink, blockdev, blkid, fsck, xfs_check
#
# ext2/ext3: resize2fs, tune2fs
# reiserfs: resize_reiserfs, reiserfstune
# xfs: xfs_growfs, xfs_info
#

TOOL=fsadm

PATH=/sbin:/usr/sbin:/bin:/usr/sbin:$PATH

# utilities
TUNE_EXT=tune2fs
RESIZE_EXT=resize2fs
TUNE_REISER=reiserfstune
RESIZE_REISER=resize_reiserfs
TUNE_XFS=xfs_info
RESIZE_XFS=xfs_growfs

MOUNT=mount
UMOUNT=umount
MKDIR=mkdir
RMDIR=rmdir
BLOCKDEV=blockdev
BLKID=blkid
GREP=grep
CUT=cut
READLINK=readlink
READLINK_E="-e"
FSCK=fsck
XFS_CHECK=xfs_check

LVRESIZE=lvresize

YES=
DRY=0
VERB=
FORCE=
EXTOFF=0
DO_LVRESIZE=0
FSTYPE=unknown
VOLUME=unknown
TEMPDIR="${TMPDIR:-/tmp}/${TOOL}_${RANDOM}$$/m"
BLOCKSIZE=
BLOCKCOUNT=
MOUNTPOINT=
MOUNTED=
REMOUNT=

IFS_OLD=$IFS

tool_usage() {
	echo "${TOOL}: Utility to resize or check the filesystem on a device"
	echo
	echo "  ${TOOL} [options] check device"
	echo "    - Check the filesystem on device using fsck"
	echo
	echo "  ${TOOL} [options] resize device [new_size[BKMGTPE]]"
	echo "    - Change the size of the filesystem on device to new_size"
	echo
	echo "  Options:"
	echo "    -h | --help         Show this help message"
	echo "    -v | --verbose      Be verbose"
	echo "    -e | --ext-offline  unmount filesystem before Ext2/3 resize"
	echo "    -f | --force        Bypass sanity checks"
	echo "    -n | --dry-run      Print commands without running them"
	echo "    -l | --lvresize     Resize given device (if it is LVM device)"
	echo "    -y | --yes          Answer \"yes\" at any prompts"
	echo
	echo "  new_size - Absolute number of filesystem blocks to be in the filesystem,"
	echo "             or an absolute size using a suffix (in powers of 1024)."
	echo "             If new_size is not supplied, the whole device is used."

	exit
}

verbose() {
	test -n "$VERB" && echo "$TOOL: $@" || true
}

error() {
	echo "$TOOL: $@" >&2
	cleanup 1
}

dry() {
	if [ "$DRY" -ne 0 ]; then
		verbose "Dry execution $@"
		return 0
	fi
	verbose "Executing $@"
	$@
}

cleanup() {
	trap '' 2
	# reset MOUNTPOINT - avoid recursion
	test "$MOUNTPOINT" = "$TEMPDIR" && MOUNTPOINT="" temp_umount
	if [ -n "$REMOUNT" ]; then
		verbose "Remounting unmounted filesystem back"
		dry $MOUNT "$VOLUME" "$MOUNTED"
	fi
	IFS=$IFS_OLD
	trap 2

	# start LVRESIZE with the filesystem modification flag
	# and allow recursive call of fsadm
	unset FSADM_RUNNING
	test "$DO_LVRESIZE" -eq 2 && exec $LVRESIZE $VERB -r -L$(( $NEWSIZE / 1048576 )) $VOLUME
	exit ${1:-0}
}

# convert parameter from Exa/Peta/Tera/Giga/Mega/Kilo/Bytes and blocks
# (2^(60/50/40/30/20/10/0))
decode_size() {
	case "$1" in
	 *[eE]) NEWSIZE=$(( ${1%[eE]} * 1152921504606846976 )) ;;
	 *[pP]) NEWSIZE=$(( ${1%[pP]} * 1125899906842624 )) ;;
	 *[tT]) NEWSIZE=$(( ${1%[tT]} * 1099511627776 )) ;;
	 *[gG]) NEWSIZE=$(( ${1%[gG]} * 1073741824 )) ;;
	 *[mM]) NEWSIZE=$(( ${1%[mM]} * 1048576 )) ;;
	 *[kK]) NEWSIZE=$(( ${1%[kK]} * 1024 )) ;;
	 *[bB]) NEWSIZE=${1%[bB]} ;;
	     *) NEWSIZE=$(( $1 * $2 )) ;;
	esac
	#NEWBLOCKCOUNT=$(round_block_size $NEWSIZE $2)
	NEWBLOCKCOUNT=$(( $NEWSIZE / $2 ))

	if [ $DO_LVRESIZE -eq 1 ]; then
		# start lvresize, but first cleanup mounted dirs
		DO_LVRESIZE=2
		cleanup 0
	fi
}

# detect filesystem on the given device
# dereference device name if it is symbolic link
detect_fs() {
        VOLUME=${1#/dev/}
	VOLUME=$($READLINK $READLINK_E -n "/dev/$VOLUME") || error "Cannot get readlink $1"
	# use /dev/null as cache file to be sure about the result
	# use 'cut' to be compatible with older version of blkid that does not provide option '-o value'
	FSTYPE=$($BLKID -c /dev/null -s TYPE "$VOLUME" | $CUT -d \" -f 2) || error "Cannot get FSTYPE of \"$VOLUME\""
	verbose "\"$FSTYPE\" filesystem found on \"$VOLUME\""
}

# check if the given device is already mounted and where
detect_mounted()  {
	$MOUNT >/dev/null || error "Cannot detect mounted device $VOLUME"
	MOUNTED=$($MOUNT | $GREP "$VOLUME")
	MOUNTED=${MOUNTED##* on }
	MOUNTED=${MOUNTED% type *} # allow type in the mount name
	test -n "$MOUNTED"
}

# get the full size of device in bytes
detect_device_size() {
	# check if blockdev supports getsize64
	$BLOCKDEV 2>&1 | $GREP getsize64 >/dev/null 
	if test $? -eq 0; then 
		DEVSIZE=$($BLOCKDEV --getsize64 "$VOLUME") || error "Cannot read size of device \"$VOLUME\""
	else
		DEVSIZE=$($BLOCKDEV --getsize "$VOLUME") || error "Cannot read size of device \"$VOLUME\""
		SSSIZE=$($BLOCKDEV --getss "$VOLUME") || error "Cannot block size read device \"$VOLUME\""
		DEVSIZE=$(($DEVSIZE * $SSSIZE))
	fi
}

# round up $1 / $2
# could be needed to gaurantee 'at least given size'
# but it makes many troubles
round_up_block_size() {
	echo $(( ($1 + $2 - 1) / $2 ))
}

temp_mount() {
	dry $MKDIR -p -m 0000 "$TEMPDIR" || error "Failed to create $TEMPDIR"
	dry $MOUNT "$VOLUME" "$TEMPDIR" || error "Failed to mount $TEMPDIR"
}

temp_umount() {
	dry $UMOUNT "$TEMPDIR" || error "Failed to umount $TEMPDIR"
	dry $RMDIR "${TEMPDIR}" || error "Failed to remove $TEMPDIR"
	dry $RMDIR "${TEMPDIR%%m}" || error "Failed to remove ${TEMPDIR%%m}"
}

yes_no() {
	echo -n "$@? [Y|n] "
	if [ -n "$YES" ]; then
		ANS="y"; echo -n $ANS
	else
		read -n 1 ANS
	fi
	test -n "$ANS" && echo
	case "$ANS" in
	  "y" | "Y" | "" ) return 0 ;;
	esac
	return 1
}

try_umount() {
	yes_no "Do you want to unmount \"$MOUNTED\"" && dry $UMOUNT "$MOUNTED" && return 0
	error "Cannot proceed test with mounted filesystem \"$MOUNTED\""
}

validate_parsing() {
	test -n "$BLOCKSIZE" -a -n "$BLOCKCOUNT" || error "Cannot parse $1 output"
}
####################################
# Resize ext2/ext3 filesystem
# - unmounted or mounted for upsize
# - unmounted for downsize
####################################
resize_ext() {
	verbose "Parsing $TUNE_EXT -l \"$VOLUME\""
	for i in $($TUNE_EXT -l "$VOLUME"); do
		case "$i" in
		  "Block size"*) BLOCKSIZE=${i##*  } ;;
		  "Block count"*) BLOCKCOUNT=${i##*  } ;;
		esac
	done
	validate_parsing $TUNE_EXT
	decode_size $1 $BLOCKSIZE
	FSFORCE=$FORCE

	if [ "$NEWBLOCKCOUNT" -lt "$BLOCKCOUNT" -o "$EXTOFF" -eq 1 ]; then
		detect_mounted && verbose "$RESIZE_EXT needs unmounted filesystem" && try_umount
		REMOUNT=$MOUNTED
		# CHECKME: after umount resize2fs requires fsck or -f flag.
		FSFORCE="-f"
	fi

	verbose "Resizing \"$VOLUME\" $BLOCKCOUNT -> $NEWBLOCKCOUNT blocks ($NEWSIZE bytes, bs:$BLOCKSIZE)"
	dry $RESIZE_EXT $FSFORCE "$VOLUME" $NEWBLOCKCOUNT
}

#############################
# Resize reiserfs filesystem
# - unmounted for upsize
# - unmounted for downsize
#############################
resize_reiser() {
	detect_mounted
	if [ -n "$MOUNTED" ]; then
		verbose "ReiserFS resizes only unmounted filesystem"
		try_umount
		REMOUNT=$MOUNTED
	fi
	verbose "Parsing $TUNE_REISER \"$VOLUME\""
	for i in $($TUNE_REISER "$VOLUME"); do
		case "$i" in
		  "Blocksize"*) BLOCKSIZE=${i##*: } ;;
		  "Count of blocks"*) BLOCKCOUNT=${i##*: } ;;
		esac
	done
	validate_parsing $TUNE_REISER
	decode_size $1 $BLOCKSIZE
	verbose "Resizing \"$VOLUME\" $BLOCKCOUNT -> $NEWBLOCKCOUNT blocks ($NEWSIZE bytes, bs: $NEWBLOCKCOUNT)"
	if [ -n "$YES" ]; then
		dry echo y | $RESIZE_REISER -s $NEWSIZE "$VOLUME"
	else
		dry $RESIZE_REISER -s $NEWSIZE "$VOLUME"
	fi
}

########################
# Resize XFS filesystem
# - mounted for upsize
# - can not downsize
########################
resize_xfs() {
	detect_mounted
	MOUNTPOINT=$MOUNTED
	if [ -z "$MOUNTED" ]; then
		MOUNTPOINT=$TEMPDIR
		temp_mount || error "Cannot mount Xfs filesystem"
	fi
	verbose "Parsing $TUNE_XFS \"$MOUNTPOINT\""
	for i in $($TUNE_XFS "$MOUNTPOINT"); do
		case "$i" in
		  "data"*) BLOCKSIZE=${i##*bsize=} ; BLOCKCOUNT=${i##*blocks=} ;;
		esac
	done
	BLOCKSIZE=${BLOCKSIZE%%[^0-9]*}
	BLOCKCOUNT=${BLOCKCOUNT%%[^0-9]*}
	validate_parsing $TUNE_XFS
	decode_size $1 $BLOCKSIZE
	if [ $NEWBLOCKCOUNT -gt $BLOCKCOUNT ]; then
		verbose "Resizing Xfs mounted on \"$MOUNTPOINT\" to fill device \"$VOLUME\""
		dry $RESIZE_XFS $MOUNTPOINT
	elif [ $NEWBLOCKCOUNT -eq $BLOCKCOUNT ]; then
		verbose "Xfs filesystem already has the right size"
	else
		error "Xfs filesystem shrinking is unsupported"
	fi
}

####################
# Resize filesystem
####################
resize() {
	NEWSIZE=$2
	detect_fs "$1"
	detect_device_size
	verbose "Device \"$VOLUME\" has $DEVSIZE bytes"
	# if the size parameter is missing use device size
	#if [ -n "$NEWSIZE" -a $NEWSIZE <
	test -z "$NEWSIZE" && NEWSIZE=${DEVSIZE}b
	trap cleanup 2
	#IFS=$'\n'  # don't use bash-ism ??
	IFS="$(printf \"\\n\")"  # needed for parsing output
	case "$FSTYPE" in
	  "ext3"|"ext2") resize_ext $NEWSIZE ;;
	  "reiserfs") resize_reiser $NEWSIZE ;;
	  "xfs") resize_xfs $NEWSIZE ;;
	  *) error "Filesystem \"$FSTYPE\" on device \"$VOLUME\" is not supported by this tool" ;;
	esac || error "Resize $FSTYPE failed"
	cleanup 0
}

###################
# Check filesystem
###################
check() {
	detect_fs "$1"
	case "$FSTYPE" in
	  "xfs") dry $XFS_CHECK "$VOLUME" ;;
	  *) dry $FSCK $YES "$VOLUME" ;;
	esac
}

#############################
# start point of this script
# - parsing parameters
#############################

# test if we are not invoked recursively
test -n "$FSADM_RUNNING" && exit 0

# test some prerequisities
test -n "$TUNE_EXT" -a -n "$RESIZE_EXT" -a -n "$TUNE_REISER" -a -n "$RESIZE_REISER" \
  -a -n "$TUNE_XFS" -a -n "$RESIZE_XFS" -a -n "$MOUNT" -a -n "$UMOUNT" -a -n "$MKDIR" \
  -a -n "$RMDIR" -a -n "$BLOCKDEV" -a -n "$BLKID" -a -n "$GREP" -a -n "$READLINK" \
  -a -n "$FSCK" -a -n "$XFS_CHECK" -a -n "LVRESIZE" -a -n "$CUT" \
  || error "Required command definitions in the script are missing!"

$($READLINK -e -n / >/dev/null 2>&1) || READLINK_E="-f"
TEST64BIT=$(( 1000 * 1000000000000 ))
test $TEST64BIT -eq 1000000000000000 || error "Shell does not handle 64bit arithmetic"
$(echo Y | $GREP Y >/dev/null) || error "Grep does not work properly"


if [ "$1" = "" ] ; then
	tool_usage
fi

while [ "$1" != "" ]
do
	case "$1" in
	 "-h"|"--help") tool_usage ;;
	 "-v"|"--verbose") VERB="-v" ;;
	 "-n"|"--dry-run") DRY=1 ;;
	 "-f"|"--force") FORCE="-f" ;;
	 "-e"|"--ext-offline") EXTOFF=1 ;;
	 "-y"|"--yes") YES="-y" ;;
	 "-l"|"--lvresize") DO_LVRESIZE=1 ;;
	 "check") shift; CHECK=$1 ;;
	 "resize") shift; RESIZE=$1; shift; NEWSIZE=$1 ;;
	 *) error "Wrong argument \"$1\". (see: $TOOL --help)"
	esac
	shift
done

if [ -n "$CHECK" ]; then
	check "$CHECK"
elif [ -n "$RESIZE" ]; then
	export FSADM_RUNNING="fsadm"
	resize "$RESIZE" "$NEWSIZE"
else
	error "Missing command. (see: $TOOL --help)"
fi
