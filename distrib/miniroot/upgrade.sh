#!/bin/sh
#	$NetBSD: upgrade.sh,v 1.24 2019/04/04 20:51:35 christos Exp $
#
# Copyright (c) 1996-2000,2006 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jason R. Thorpe.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

#	NetBSD installation script.
#	In a perfect world, this would be a nice C program, with a reasonable
#	user interface.

ROOTDISK=""				# filled in below

trap "unmount_fs -fast /tmp/fstab.shadow > /dev/null 2>&1; rm -f /tmp/fstab.shadow" 0

MODE="upgrade"

# include machine-dependent functions
# The following functions must be provided:
#	md_copy_kernel()	- copy a kernel to the installed disk
#	md_get_diskdevs()	- return available disk devices
#	md_get_cddevs()		- return available CD-ROM devices
#	md_get_ifdevs()		- return available network interfaces
#	md_get_partition_range() - return range of valid partition letters
#	md_installboot()	- install boot-blocks on disk
#	md_labeldisk()		- put label on a disk
#	md_welcome_banner()	- display friendly message
#	md_not_going_to_install() - display friendly message
#	md_congrats()		- display friendly message
#	md_set_term		- set terminal type
#	md_makerootwritable	- what it says
# optional:
#	md_upgrade_prep_needed	- variable: set if you md_prepare_upgrade()
#	md_prepare_upgrade	- any machine dependent preparations
#	md_view_labels_possible	- variable: md_view_labels defined
#	md_view_labels		- peek at preexisting disk labels, to 
#				  better identify disks

# we need to make sure .'s below work if this directory is not in $PATH
# dirname may not be available but expr is
Mydir=$(expr $0 : '^\(.*\)/[^/]*$')
Mydir=$(cd ${Mydir:-.}; pwd)

# this is the most likely place to find the binary sets
# so save them having to type it in
Default_sets_dir=$Mydir/../../binary/sets

# include machine dependent subroutines
. $Mydir/install.md

# include common subroutines
. $Mydir/install.sub

# which sets?
THESETS="$UPGRSETS $MDSETS"

# Files that moved between 1.2 and 1.3
RELOCATED_FILES_13="${RELOCATED_FILES_13} /sbin/dumpfs /usr/sbin/dumpfs"
RELOCATED_FILES_13="${RELOCATED_FILES_13} /sbin/dumplfs /usr/sbin/dumplfs"
RELOCATED_FILES_13="${RELOCATED_FILES_13} /sbin/nfsd /usr/sbin/nfsd"
RELOCATED_FILES_13="${RELOCATED_FILES_13} /sbin/nfsiod /usr/sbin/nfsiod"
RELOCATED_FILES_13="${RELOCATED_FILES_13} /sbin/mountd /usr/sbin/mountd"
RELOCATED_FILES_13="${RELOCATED_FILES_13} /sbin/quotacheck /usr/sbin/quotacheck"
RELOCATED_FILES_13="${RELOCATED_FILES_13} /sbin/rtquery /usr/sbin/rtquery"

# Files that moved between 1.3 and 1.4
RELOCATED_FILES_14="${RELOCATED_FILES_14} /sbin/newlfs /sbin/newfs_lfs"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/games/chess /usr/games/chess"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/games/hide/chess /usr/games/hide/chess"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/share/games/gnuchess.book /usr/share/games/gnuchess.book"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/share/games/atc/ATC_scores /usr/share/games/atc/ATC_scores"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/share/games/atc/ATC_scores /usr/share/games/atc/ATC_scores"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/lib/libamu.a /usr/lib/libamu.a"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/lib/libamu_p.a /usr/lib/libamu_p.a"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/lib/libamu_pic.a /usr/lib/libamu_pic.a"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/lib/libbfd.a /usr/lib/libbfd.a"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/lib/libbfd_pic.a /usr/lib/libbfd_pic.a"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/lib/libg++.a /usr/lib/libg++.a"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/lib/libg++_p.a /usr/lib/libg++_p.a"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/lib/libg++_pic.a /usr/lib/libg++_pic.a"

RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/sbin/gettable /usr/sbin/gettable"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/sbin/dig /usr/bin/dig"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/sbin/dnsquery /usr/bin/dnsquery"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/sbin/htable /usr/bin/htable"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/sbin/htable /usr/sbin/htable"
RELOCATED_FILES_14="${RELOCATED_FILES_14} /usr/sbin/nslookup /usr/sbin/nslookup"

rm_relocated_files()
{
	# ($n, $(n+1)): pairs of (old,new) locations of relocated files
	while [ $# -ge 2 ]; do
		if [ -f "$2" ]; then
			echo Removing "$1";
			rm -f "$1"
		fi
		shift 2
	done
}

# Good {morning,afternoon,evening,night}.
md_welcome_banner
echo -n "Proceed with upgrade? [n] "
getresp "n"
case "$resp" in
	y*|Y*)
		echo	"Cool!  Let's get to it..."
		;;
	*)
		md_not_going_to_install
		exit
		;;
esac

# Deal with terminal issues
md_set_term

# XXX Work around vnode aliasing bug (thanks for the tip, Chris...)
ls -l /dev > /dev/null 2>&1

# Make sure we can write files (at least in /tmp)
# This might make an MFS mount on /tmp, or it may
# just re-mount the root with read-write enabled.
md_makerootwritable

test "$md_view_labels_possible" && md_view_labels

while [ -z "${ROOTDISK}" ]; do
	getrootdisk
done

# Assume partition 'a' of $ROOTDISK is for the root filesystem.  Confirm
# this with the user.  Check and mount the root filesystem.
resp=""			# force one iteration
while [ -z "${resp}" ]; do
	echo -n	"Root filesystem? [${ROOTDISK}a] "
	getresp "${ROOTDISK}a"
	_root_filesystem="/dev/$(basename $resp)"
	if [ ! -b ${_root_filesystem} ]; then
		echo "Sorry, ${resp} is not a block device."
		resp=""	# force loop to repeat
	fi
done

echo	"Checking root filesystem..."
if ! fsck -pf ${_root_filesystem}; then
	echo	"ERROR: can't check root filesystem!"
	exit 1
fi

echo	"Mounting root filesystem..."
if ! mount -o ro ${_root_filesystem} /mnt; then
	echo	"ERROR: can't mount root filesystem!"
	exit 1
fi

# Grab the fstab so we can munge it for our own use.
if [ ! -f /mnt/etc/fstab ]; then
	echo	"ERROR: no /etc/fstab!"
	exit 1
fi

# Grab the hosts table so we can use it.
if [ ! -f /mnt/etc/hosts ]; then
	echo	"ERROR: no /etc/hosts!"
	exit 1
fi
cp /mnt/etc/hosts /tmp/hosts

# Start up the network in same/similar configuration as the installed system
# uses.
cat << \__network_config_1

The upgrade program would now like to enable the network.  It will use the
configuration already stored on the root filesystem.  This is required
if you wish to use the network installation capabilities of this program.

__network_config_1
echo -n	"Enable network? [y] "
getresp "y"
case "$resp" in
	y*|Y*)
		if ! enable_network; then
			echo "ERROR: can't enable network!"
			exit 1
		fi

		cat << \__network_config_2

You will now be given the opportunity to escape to the command shell to
do any additional network configuration you may need.  This may include
adding additional routes, if needed.  In addition, you might take this
opportunity to redo the default route in the event that it failed above.

__network_config_2
		echo -n "Escape to shell? [n] "
		getresp "n"
		case "$resp" in
			y*|Y*)
				echo "Type 'exit' to return to upgrade."
				sh
				;;

			*)
				;;
		esac
		;;
	*)
		;;
esac

# Now that the network has been configured, it is safe to configure the
# fstab.  We remove all but ufs/ffs.
(
	> /tmp/fstab
	while read _dev _mp _fstype _rest ; do
		if [ "${_fstype}" = "ufs" ] ||
		   [ "${_fstype}" = "ffs" ]; then
			if [ "${_fstype}" = "ufs" ]; then
				# Convert ufs to ffs.
				_fstype=ffs
			fi
			echo "$_dev $_mp $_fstype $_rest" >> /tmp/fstab
		fi
	done
) < /mnt/etc/fstab

echo	"The fstab is configured as follows:"
echo	""
cat /tmp/fstab
cat << \__fstab_config_1

You may wish to edit the fstab.  For example, you may need to resolve
dependencies in the order which the filesystems are mounted.  Note that
this fstab is only for installation purposes, and will not be copied into
the root filesystem.

__fstab_config_1
echo -n	"Edit the fstab? [n] "
getresp "n"
case "$resp" in
	y*|Y*)
		${EDITOR} /tmp/fstab
		;;

	*)
		;;
esac

echo	""
munge_fstab /tmp/fstab /tmp/fstab.shadow

if ! umount /mnt; then
	echo	"ERROR: can't unmount previously mounted root!"
	exit 1
fi

# Check all of the filesystems.
check_fs /tmp/fstab.shadow

# Mount filesystems.
mount_fs /tmp/fstab.shadow

# Machine dependent preparation.
test "$md_upgrade_prep_needed" && {
	md_prepare_upgrade || {
		cat << 'EOF'
The preparations for upgrading your machine did not complete successfully.

EOF
		echo -n "Continue anyway? [n]"
		getresp "n"
		case "$resp" in
			y*|Y*)
				;;
			*)
				exit 1
				;;
		esac
	}
}


echo -n	"Are the upgrade sets on one of your normally mounted (local) filesystems? [y] "
getresp "y"
case "$resp" in
	y*|Y*)
		get_localdir /mnt
		;;
	*)
		;;
esac

# Install sets.
install_sets

# Remove files that have just been installed in a new location
# from the old location
rm_relocated_files $(eval echo \\$RELOCATED_FILES_${VERSION})

# Get timezone info
get_timezone

# Fix up the fstab.
echo -n	"Converting ufs to ffs in /etc/fstab..."
(
	> /tmp/fstab
	while read _dev _mp _fstype _rest ; do
		if [ "${_fstype}" = "ufs" ]; then
			# Convert ufs to ffs.
			_fstype=ffs
		fi
		echo "$_dev $_mp $_fstype $_rest" >> /tmp/fstab
	done
) < /mnt/etc/fstab
echo	"done."
echo -n	"Would you like to edit the resulting fstab? [y] "
getresp "y"
case "$resp" in
	y*|Y*)
		${EDITOR} /tmp/fstab
		;;

	*)
		;;
esac

# Copy in configuration information and make devices in target root.
(
	cd /tmp
	for file in fstab; do
		if [ -f $file ]; then
			echo -n "Copying $file..."
			cp $file /mnt/etc/$file
			echo "done."
		fi
	done

	echo -n "Installing timezone link..."
	rm -f /mnt/etc/localtime
	ln -s /usr/share/zoneinfo/$TZ /mnt/etc/localtime
	echo "done."

	echo -n "Making devices..."
	_pid=$(twiddle)
	cd /mnt/dev
	sh MAKEDEV all
	kill $_pid
	echo "done."

	md_copy_kernel

	md_installboot ${ROOTDISK}
)

unmount_fs /tmp/fstab.shadow

# Pat on the back.
md_congrats

# ALL DONE!
exit 0
