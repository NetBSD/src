#	$NetBSD: install.md,v 1.4 2017/11/25 09:41:45 tsutsui Exp $
#
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
# machine dependent section of installation/upgrade script.
#

VERSION=				# filled in automatically (see list)
export VERSION

md_set_term() {
	if [ ! -z "$TERM" ]; then
		return
	fi
	echo -n "Specify terminal type [vt220]: "
	getresp "vt220"
	TERM="$resp"
	export TERM
}

__mount_kernfs() {
	# Make sure kernfs is mounted.
	if [ ! -d /kern -o ! -e /kern/msgbuf ]; then
		mkdir /kern > /dev/null 2>&1
		/sbin/mount_kernfs /kern /kern
	fi
}

md_makerootwritable() {
	# Mount root rw for convenience of the tester ;-)
	if [ ! -e /tmp/.root_writable ]; then
		__mount_kernfs
		mount -u /kern/rootdev /
		cp /dev/null /tmp/.root_writable
	fi
}

md_get_diskdevs() {
	# return available disk devices
	__mount_kernfs
	sed -n -e '/^sd[0-9] /s/ .*//p' \
	       -e '/^wd[0-9] /s/ .*//p' \
		< /kern/msgbuf | sort -u
}

md_prep_disklabel()
{
	# $1 is the root disk
	# Note that the first part of this function is just a *very* verbose
	# version of md_label_disk().

	cat << \__md_prep_disklabel_1
You now have to prepare your root disk for the installation of NetBSD. This
is further referred to as 'labeling' a disk.

First you get the chance to edit or create an AHDI compatible partitioning on
the installation disk. Note that NetBSD can do without AHDI partitions,
check the documentation.
If you want to use an AHDI compatible partitioning, you have to assign some
partitions to NetBSD before NetBSD is able to use the disk. Change the 'id'
of all partitions you want to use for NetBSD filesystems to 'NBD'. Change
the 'id' of the partition you wish to use for swap to 'SWP'.

Hit the <return> key when you have read this...
__md_prep_disklabel_1
	getresp ""
	ahdilabel /dev/r${1}c

	# display example
	cat << \__md_prep_disklabel_3
Here is an example of what the partition information will look like once
you have entered the disklabel editor. Disk partition sizes and offsets
are in sector (most likely 512 bytes) units.

[Example]
partition      start         (c/t/s)      nblks         (c/t/s)  type

 a (root)          0       (0/00/00)      31392     (109/00/00)  4.2BSD
 b (swap)      31392     (109/00/00)      73440     (255/00/00)  swap
 c (disk)          0       (0/00/00)    1070496    (3717/00/00)  unused
 d (user)     104832     (364/00/00)      30528     (106/00/00)  4.2BSD
 e (user)     135360     (470/00/00)      40896     (142/00/00)  4.2BSD
 f (user)     176256     (612/00/00)      92160     (320/00/00)  4.2BSD
 g (user)     268416     (932/00/00)     802080    (2785/00/00)  4.2BSD

[End of example]

Hit the <return> key when you have read this...

__md_prep_disklabel_3
	getresp ""
	edlabel /dev/r${1}c

	cat << \__md_prep_disklabel_4

You will now be given the opportunity to place disklabels on any additional
disks on your system.
__md_prep_disklabel_4

	_DKDEVS=`rmel ${1} ${_DKDEVS}`
	resp="X"	# force at least one iteration
	while [ "X$resp" != X"done" ]; do
		labelmoredisks
	done
}

md_labeldisk() {
	edahdi /dev/r${1}c < /dev/null > /dev/null 2>&1
	[ $? -eq 0 ] && edahdi /dev/r${1}c
	edlabel /dev/r${1}c
}

md_welcome_banner() {
	echo ""
	echo "Welcome to the NetBSD/atari ${VERSION} preparation program."
		cat << \__welcome_banner_1

This program is designed to partition your disk in preparation of the
NetBSD installation. At this stage, the only thing you _must_ setup
is a swap partition. If you wish, the remaining partitioning work might
be delayed until the actual installation.

As with anything which modifies your disk's contents, this
program can cause SIGNIFICANT data loss, and you are advised
to make sure your data is backed up before beginning the
preparation process.

Default answers are displayed in brackets after the questions.
You can hit Control-C at any time to quit, but if you do so at a
prompt, you may have to hit return.
__welcome_banner_1
}

md_not_going_to_install() {
	cat << \__not_going_to_install_1

OK, then.  Enter `halt' at the prompt to halt the machine.  Once the
machine has halted, power-cycle the system to load new boot code.

Note: If you wish to have another try. Just type '^D' at the prompt. After
      a moment, the program will restart itself.

__not_going_to_install_1
}

md_congrats() {
	cat << __congratulations_1

CONGRATULATIONS!  You have successfully partitioned your disks!
Now you can use file2swap.ttp to transfer the install.fs to your
swap partition and continue the installation.
Enter halt at the command prompt. Once the system has halted, reset the
machine and re-boot it.

Note: If you wish to have another try. Just type '^D' at the prompt. After
      a moment, the installer will restart itself.

__congratulations_1
}
