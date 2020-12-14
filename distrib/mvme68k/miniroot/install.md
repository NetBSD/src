#!/bin/sh
#
#	$NetBSD: install.md,v 1.8.2.1 2020/12/14 17:26:37 martin Exp $
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

#
# machine dependent section of installation/upgrade script
#

# Machine-dependent install sets
MDSETS="kern-GENERIC xbase xcomp xetc xfont xserver"

md_set_term() {
	if [ ! -z "$TERM" ]; then
		return
	fi
	echo -n "Specify terminal type [vt100]: "
	getresp "vt100"
	TERM="$resp"
	export TERM
	# XXX call tset?
}

md_makerootwritable() {
	# Just remount the root device read-write.
	mi_mount_kernfs
	echo "Remounting root read-write..."
	mount -t ffs -u /kern/rootdev /
}

md_get_diskdevs() {
	# return available disk devices
	mi_mount_kernfs
	mi_filter_msgbuf | sed -ne '/^sd[0-9] /s/ .*//p'
}

md_get_cddevs() {
	# return available CDROM devices
	mi_mount_kernfs
	mi_filter_msgbuf | sed -ne '/^cd[0-9] /s/ .*//p'
}

md_get_ifdevs() {
	# return available network devices
	mi_filter_msgbuf | sed -ne '/^[il]e[0-9] /s/ .*//p'
}

md_get_partition_range() {
	# return an expression describing the valid partition id's
	echo '[a-h]'
}

md_installboot() {
	# install the boot block on disk $1
	echo "Installing boot block..."
	( cd /usr/mdec ;\
	cp -p ./bootsd /mnt/.bootsd ;\
	sync ; sleep 1 ; sync ;\
	./installboot -v /mnt/.bootsd bootxx /dev/r${1}a )
	echo "done."
}

md_native_fstype() {
}

md_native_fsopts() {
}

grep_check () {
	pattern=$1; shift
	awk 'BEGIN{ es=1; } /'"$pattern"'/{ print; es=0; } END{ exit es; }' "$@"
}

md_checkfordisklabel() {
	# $1 is the disk to check
	local rval

	disklabel $1 > /dev/null 2> /tmp/checkfordisklabel
	if grep_check "no disklabel" /tmp/checkfordisklabel; then
		rval=1
	elif grep_check "disk label corrupted" /tmp/checkfordisklabel; then
		rval=2
	else
		rval=0
	fi

	rm -f /tmp/checkfordisklabel
	return $rval
}

md_prep_disklabel()
{
	local _disk

	_disk=$1
	md_checkfordisklabel $_disk
	case $? in
	0)
		echo -n "Do you wish to edit the disklabel on $_disk? [y]"
		;;
	1)
		echo "WARNING: Disk $_disk has no label"
		echo -n "Do you want to create one with the disklabel editor? [y]"
		;;
	2)
		echo "WARNING: Label on disk $_disk is corrupted"
		echo -n "Do you want to try and repair the damage using the disklabel editor? [y]"
		;;
	esac

	getresp "y"
	case "$resp" in
	y*|Y*) ;;
	*)	return ;;
	esac

	# display example
	cat << \__md_prep_disklabel_1

Here is an example of what the partition information will look like once
you have entered the disklabel editor. Disk partition sizes and offsets
are in sector (most likely 512 bytes) units. Make sure these size/offset
pairs are on cylinder boundaries (the number of sector per cylinder is
given in the 'sectors/cylinder' entry, which is not shown here).

Do not change any parameters except the partition layout and the label name.
It's probably also wisest not to touch the '8 partitions:' line, even
in case you have defined less than eight partitions.

[Example]
8 partitions:
#        size   offset    fstype   [fsize bsize   cpg]
  a:    50176        0    4.2BSD     1024  8192    16   # (Cyl.    0 - 111)
  b:    64512    50176      swap                        # (Cyl.  112 - 255)
  c:   640192        0   unknown                        # (Cyl.    0 - 1428)
  d:   525504   114688    4.2BSD     1024  8192    16   # (Cyl.  256 - 1428)
[End of example]

__md_prep_disklabel_1
	echo -n "Press [Enter] to continue "
	getresp ""
	disklabel -i -I /dev/r${_disk}c
}

md_copy_kernel() {
	echo -n "Copying kernel..."
	cp -p /netbsd /mnt/netbsd
	echo "done."
}

md_welcome_banner() {
	echo	"Welcome to the NetBSD/${MACHINE} ${RELEASE} installation program."
	cat << \__welcome_banner_1

This program is designed to help you install NetBSD on your system in a simple
and rational way.  You'll be asked several questions, and it would probably be
useful to have your disk's hardware manual, the installation notes, and a
calculator handy.

In particular, you will need to know some reasonably detailed information
about your disk's geometry. The kernel will attempt to display geometry
information for SCSI disks during boot, if possible. If you did not make it
note of it before, you may wish to reboot and jot down your disk's geometry
before proceeding.

As with anything which modifies your hard disk's contents, this program can
cause SIGNIFICANT data loss, and you are advised to make sure your hard drive
is backed up before beginning the installation process.

Default answers are displyed in brackets after the questions. You can hit
Control-C at any time to quit, but if you do so at a prompt, you may have to
hit return.  Also, quitting in the middle of installation may leave your
system in an inconsistent state.
__welcome_banner_1
}

md_not_going_to_install() {
		cat << \__not_going_to_install_1

OK, then.  Enter 'halt' at the prompt to halt the machine.  Once the
machine has halted, power-cycle the system to load new boot code.

__not_going_to_install_1
}

md_congrats() {
	cat << \__congratulations_1

CONGRATULATIONS!  You have successfully installed NetBSD!  To boot the
installed system, enter halt at the command prompt.  Once the system has
halted, power-cycle the machine in order to load new boot code.  Make sure
you boot from the root disk.

__congratulations_1
}

md_native_fstype() {
	# Nothing to do.
}

md_native_fsopts() {
	# Nothing to do.
}
