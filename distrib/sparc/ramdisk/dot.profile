# $NetBSD: dot.profile,v 1.11 2000/10/20 11:56:58 pk Exp $
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

if [ "${BOOTFS_DONEPROFILE}" = "YES" ]; then
	exit
fi

BOOTFS_DONEPROFILE=YES
export BOOTFS_DONEPROFILE

# mount root read-write
mount -u /dev/md0a /

# mount /instfs
MINIROOT_FSSIZE=10000
mount -t mfs -o -s=$MINIROOT_FSSIZE swap /instfs

# Load instfs
echo ""
echo "If you booted the system from a floppy you can now remove the disk."
echo ""
echo "Next, insert the floppy disk labeled \`instfs' into the disk drive."
echo "The question below allows you to specify the device name of the floppy"
echo "drive.  Usually, the default answer \`floppy' will do just fine."
echo ""

_dev="floppy"
while :; do
	#if [ "$_dev" = "" ] ; then _dev=floppy; fi
	echo -n "Device to load the \`instfs' filesystem from [$_dev]: "
	read _answer
	if [ "$_answer" != "" ]; then _dev=$_answer; fi
	if [ "$_dev" = floppy ]; then _dev=/dev/rfd0a; fi
	if [ "$_dev" = "none" ]; then continue; fi
	(cd /instfs && tar zxvpf $_dev) && break
done

# switch to /instfs, and pretend we logged in there.
exec chroot /instfs /bin/sh /.profile
