#!/bin/sh
#
#	$NetBSD: install.md,v 1.3 2003/03/07 16:57:46 he Exp $
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

#
# machine dependent section of installation/upgrade script
#

# Machine-dependent install sets
MDSETS=""

md_set_term() {
	if [ ! -z "$TERM" ]; then
		return
	fi
	echo -n "Specify terminal type [vt220]: "
	getresp "vt220"
	TERM="$resp"
	export TERM
	# XXX call tset?
}

md_makerootwritable() {
	# Was: do_mfs_mount "/tmp" "2048"
	# /tmp is the mount point
	# 2048 is the size in DEV_BIZE blocks

	umount /tmp > /dev/null 2>&1
	if ! mount_mfs -s 2048 swap /tmp ; then
		cat << \__mfs_failed_1

FATAL ERROR: Can't mount the memory filesystem.

__mfs_failed_1
		exit
	fi

	# Bleh.  Give mount_mfs a chance to DTRT.
	sleep 2
}

md_get_diskdevs() {
	# return available disk devices
	dmesg | awk -F : '/^sd[0-9]*:.*cylinders/ { print $1; }' | sort -u
}

md_get_cddevs() {
	# return available CD-ROM devices
	dmesg | awk -F : '/cd[0-9]*:.*CD-ROM/ { print $1; }' | sort -u
}

md_get_ifdevs() {
	# return available network interfaces
	dmesg | awk -F : '/^ae[0-9]*:/ { print $1; }' | sort -u
	dmesg | awk -F : '/^sn[0-9]*:/ { print $1; }' | sort -u
}

md_installboot() {
	# $1 is the root disk

	# We don't have boot blocks.  Sigh.
}

md_checkfordisklabel() {
	# $1 is the disk to check

	# We don't support labels on the mac68k port.  (Yet.)

	rval="0"
}

md_labeldisk() {
	# $1 is the disk to label

	# We don't support labels on the mac68k port.  (Yet.)
}

md_prep_disklabel() {
	# $1 is the root disk

	# We don't support labels on the mac68k port.  (Yet.)
}

md_copy_kernel() {
	echo -n "Copying kernel..."
	cp -p /netbsd /mnt/netbsd
	echo "done."
}

	# Note, while they might not seem machine-dependent, the
	# welcome banner and the punt message may contain information
	# and/or instructions specific to the type of machine.

md_welcome_banner() {
(
	echo	""
	echo	"Welcome to the NetBSD/mac68k ${VERSION} installation program."
	cat << \__welcome_banner_1

This program is designed to help you install NetBSD on your system in a
simple and rational way.  You'll be asked several questions, and it would
probably be useful to have your disk's hardware manual, the installation
notes, and a calculator handy.

In particular, you will need to know some reasonably detailed
information about your disk's geometry.  This program can determine
some limited information about certain specific types of HP-IB disks.
If you have SCSI disks, however, prior knowledge of disk geometry
is absolutely essential.  The kernel will attempt to display geometry
information for SCSI disks during boot, if possible.  If you did not
make it note of it before, you may wish to reboot and jot down your
disk's geometry before proceeding.

As with anything which modifies your hard disk's contents, this
program can cause SIGNIFICANT data loss, and you are advised
to make sure your hard drive is backed up before beginning the
installation process.

Default answers are displyed in brackets after the questions.
You can hit Control-C at any time to quit, but if you do so at a
prompt, you may have to hit return.  Also, quitting in the middle of
installation may leave your system in an inconsistent state.

__welcome_banner_1
) | more
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
