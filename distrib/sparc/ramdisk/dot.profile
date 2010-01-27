# $NetBSD: dot.profile,v 1.18.16.1 2010/01/27 20:59:46 bouyer Exp $
#
# Copyright (c) 2000 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Paul Kranenburg.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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

PATH=/sbin:/bin:/usr/bin:/usr/sbin:/
export PATH
HOME=/
export HOME

umask 022

MACHINE=sparc
INSTFS_MP=/instfs
MINIROOT_FSSIZE=10000
MINIROOT_BPINODE=1024

if [ "${BOOTFS_DONEPROFILE}" != "YES" ]; then

	BOOTFS_DONEPROFILE=YES
	export BOOTFS_DONEPROFILE

	# mount root read-write
	mount_ffs -o update /dev/md0a /

	# mount /instfs
	mount_mfs -s $MINIROOT_FSSIZE -i $MINIROOT_BPINODE swap $INSTFS_MP
fi

# A cat simulator
cat()
{
	local l
	while read l; do
		echo "$l"
	done
}

_resp=""
getresp() {
	read _resp
	if [ "$_resp" = "" ]; then
		_resp=$1
	fi
}

# Load instfs

floppy()
{
	local dev rval

	rval=0
	dev="/dev/fd0a"

	echo "Ejecting floppy disk"
	eject $dev

	cat <<EOF
Remove the boot disk from the floppy station and insert the second disk of
the floppy installation set into the disk drive.

The question below allows you to specify the device name of the floppy
drive.  Usually, the default device will do just fine.
EOF
	echo -n "Floppy device to load the installation utilities from [$dev]: "
	getresp "$dev"; dev="$_resp"

	echo "Extracting installation utilities... "
	(cd $INSTFS_MP && tar zxpf $dev) || rval=1

	echo "Ejecting floppy disk"
	eject $dev
	return $rval
}

tape()
{
	local dev fn bsa
	cat <<EOF
By default, the installation utilities are located in the second tape file
on the NetBSD/sparc installation tape. In case your tape layout is different,
choose the appropriate tape file number below.

EOF
	dev="/dev/nrst0"
	echo -n "Tape device to load the installation utilities from [$dev]: "
	getresp "$dev"; dev="$_resp"

	fn=2
	echo -n "Tape file number [$fn]: "
	getresp "$fn"; fn="$_resp"

	echo -n "Tape block size (use only if you know you need it): "
	getresp ""; if [ "$_resp" != "" ]; then
		bsa="-b $_resp"
	fi

	echo "Positioning tape... "
	mt -f $dev asf $(($fn - 1))
	[ $? = 0 ] || return 1

	echo "Extracting installation utilities... "
	(cd $INSTFS_MP && tar $bsa -z -x -p -f $dev) || return 1
}

cdrom()
{
	local dev tf rval
	cat <<EOF
The installation utilities are located on the ISO CD9660 filesystem on the
NetBSD/sparc CD-ROM. We need to mount the filesystem from the CD-ROM device
which you can specify below. Note: after the installation utilities are
extracted this filesystem will be unmounted again.

EOF

	rval=0
	dev="/dev/cd0a"
	echo -n "CD-ROM device to use [$dev]: "
	getresp "$dev"; dev="$_resp"

	mount_cd9660 -o rdonly $dev /cdrom || return 1

	# Look for instfs.tgz in MACHINE subdirectory first
	tf=/cdrom/$MACHINE/installation/bootfs/instfs.tgz
	[ -f $tf ] || tf=/cdrom/installation/bootfs/instfs.tgz
	[ -f $tf ] || {
		echo "Note: instfs.tgz image not found in default location"
		tf=""
	}

	while :; do
		echo -n "Path to instfs.tgz [$tf] "
		[ -z "$tf" ] && echo -n "(<return> to abort) "
		getresp "$tf"; tf="$_resp"
		[ -z "$tf" ] && { rval=1; break; }
		[ -f "$tf" ] && break;
		echo "$tf not found"
		tf=""
	done

	[ $rval = 0 ] && (cd $INSTFS_MP && tar zxpf $tf) || rval=1

	umount /cdrom
	return $rval
}

cat <<EOF
Welcome to the NetBSD/sparc microroot setup utility.

We've just completed the first stage of a two-stage procedure to load a
fully functional NetBSD installation environment on your machine.  In the
second stage which is to follow now, a set of additional installation
utilities must be load from your NetBSD/sparc installation medium.

EOF

while :; do
	cat <<EOF
This procedure supports one of the following media:

	1) cdrom
	2) tape
	3) floppy

EOF
	echo -n "Installation medium to load the additional utilities from: "
	read answer
	echo ""
	case "$answer" in
		1|cdrom)	_func=cdrom;;
		2|tape)		_func=tape;;
		3|floppy)	_func=floppy;;
		*)		echo "option not supported"; continue;;
	esac
	$_func && break
done

# switch to /instfs, and pretend we logged in there.
chroot $INSTFS_MP /bin/sh /.profile

#
echo "Back in microroot; halting machine..."
halt
