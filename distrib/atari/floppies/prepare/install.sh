#!/bin/sh
#	$NetBSD: install.sh,v 1.4 2022/05/03 20:52:30 andvar Exp $
#
# Copyright (c) 1996 The NetBSD Foundation, Inc.
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

FILESYSTEMS="/tmp/filesystems"		# used throughout
MODE="install"

# include machine-dependent functions
# The following functions must be provided:
#	md_prep_disklabel()	- label the root disk
#	md_welcome_banner()	- display friendly message
#	md_congrats()		- display friendly message
#	md_makerootwritable()	- make root writable (at least /tmp)

# we need to make sure .'s below work if this directory is not in $PATH
case $0 in
*/*)	Mydir=${0%/*};;
*)	Mydir=.;;
esac
Mydir=$(cd "${Mydir}" && pwd)

#
# Sub-parts
#
getresp() {
	read resp
	if [ -z "$resp" ]; then
		resp=$1
	fi
}

isin() {
# test the first argument against the remaining ones, return succes on a match
	local a=$1
	shift
	while [ $# != 0 ]; do
		if [ "$a" = "$1" ]; then return 0; fi
		shift
	done
	return 1
}

getrootdisk() {
	cat << \__getrootdisk_1

The installation program needs to know which disk to consider
the root disk.  Note the unit number may be different than
the unit number you used in the standalone installation
program.

Available disks are:

__getrootdisk_1
	_DKDEVS=`md_get_diskdevs`
	echo	"$_DKDEVS"
	echo	""
	echo -n	"Which disk is the root disk? "
	getresp ""
	if isin $resp $_DKDEVS ; then
		ROOTDISK="$resp"
	else
		echo ""
		echo "The disk $resp does not exist."
		ROOTDISK=""
	fi
}

labelmoredisks() {
	cat << \__labelmoredisks_1

You may label the following disks:

__labelmoredisks_1
	echo "$_DKDEVS"
	echo	""
	echo -n	"Label which disk? [done] "
	getresp "done"
	case "$resp" in
		"done")
			;;

		*)
			if isin $resp $_DKDEVS ; then
				md_labeldisk $resp
			else
				echo ""
				echo "The disk $resp does not exist."
			fi
			;;
	esac
}

#
# include machine dependent subroutines
. $Mydir/install.md

# Good {morning,afternoon,evening,night}.
md_welcome_banner
echo -n "Proceed? [n] "
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

# XXX Work around vnode aliasing bug (thanks for the tip, Chris...)
ls -l /dev > /dev/null 2>&1

# Deal with terminal issues
md_set_term

# Make sure we can write files (at least in /tmp)
# This might make an MFS mount on /tmp, or it may
# just re-mount the root with read-write enabled.
md_makerootwritable

while [ "X${ROOTDISK}" = "X" ]; do
	getrootdisk
done

# Deal with disklabels, including editing the root disklabel
# and labeling additional disks.  This is machine-dependent since
# some platforms may not be able to provide this functionality.
md_prep_disklabel ${ROOTDISK}

# Pat on the back.
md_congrats

# ALL DONE!
exit 0
