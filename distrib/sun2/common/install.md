#	$NetBSD: install.md,v 1.1 2001/05/18 00:16:38 fredette Exp $
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
# machine dependent section of installation/upgrade script.
#

# Machine-dependent install sets
# MDSETS="xbin xman xinc xcon" XXX
MDSETS=""

md_set_term() {
	if [ ! -z "$TERM" ]; then
		return
	fi
	echo -n "Specify terminal type [sun]: "
	getresp "sun"
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
	# Just remount the root device read-write.
	if [ ! -e /tmp/root_writable ]; then
		echo "Remounting root read-write..."
		__mount_kernfs
		mount -u /kern/rootdev /
		swapctl -a /kern/rootdev
		cp /dev/null /tmp/root_writable
	fi
}

md_get_diskdevs() {
	# return available disk devices
	__mount_kernfs
	sed -n -e '/^sd[0-9] /s/ .*//p' \
	       -e '/^xd[0-9] /s/ .*//p' \
	       -e '/^xy[0-9] /s/ .*//p' \
		< /kern/msgbuf | sort -u
}

md_get_cddevs() {
	# return available CDROM devices
	__mount_kernfs
	sed -n -e '/^cd[0-9] /s/ .*//p' \
		< /kern/msgbuf | sort -u
}

md_get_ifdevs() {
	# return available network devices
	__mount_kernfs
	sed -n -e '/^ie[0-9] /s/ .*//p' \
	       -e '/^le[0-9] /s/ .*//p' \
		< /kern/msgbuf | sort -u
}

md_get_partition_range() {
	# return an expression describing the valid partition id's
	echo '[a-h]'
}

md_installboot() {
	# install the boot block on disk $1
	echo "Installing boot block..."
	( cd /usr/mdec ;\
	cp -p ./ufsboot /mnt/ufsboot ;\
	sync ; sleep 1 ; sync ;\
	./installboot -v /mnt/ufsboot bootxx /dev/r${1}a )
	echo "done."
}

md_native_fstype() {
}

md_native_fsopts() {
}

md_prep_disklabel() {
	# $1 is the root disk
	echo -n "Do you wish to edit the disklabel on ${1}? [y]"
	getresp "y"
	case "$resp" in
	y*|Y*) ;;
	*)	return ;;
	esac

	# display example
	cat << \__md_prep_disklabel_1
Here is an example of what the partition information will look like once
you have entered the disklabel editor. Disk partition sizes and offsets
are in sector (most likely 512 bytes) units. Make sure all partitions
start on a cylinder boundary (c/t/s == XXX/0/0).

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

__md_prep_disklabel_1
	getresp ""
	edlabel /dev/r${1}c
}

md_copy_kernel() {
	set -- `sysctl -n hw.model`
	echo -n "Copying $1 kernel..."
	cp -p /netbsd.$1 /mnt/netbsd
	echo "done."
}

md_welcome_banner() {
	if [ "$MODE" = "install" ]; then
		echo ""
		echo "Welcome to the NetBSD/sun2 ${VERSION} installation program."
		cat << \__welcome_banner_1

This program is designed to help you put NetBSD on your disk,
in a simple and rational way.  You'll be asked several questions,
and it would probably be useful to have your disk's hardware
manual, the installation notes, and a calculator handy.
__welcome_banner_1

	else
		echo ""
		echo "Welcome to the NetBSD/sun2 ${VERSION} upgrade program."
		cat << \__welcome_banner_2

This program is designed to help you upgrade your NetBSD system in a
simple and rational way.

As a reminder, installing the `etc' binary set is NOT recommended.
Once the rest of your system has been upgraded, you should manually
merge any changes to files in the `etc' set into those files which
already exist on your system.
__welcome_banner_2
	fi

cat << \__welcome_banner_3

As with anything which modifies your disk's contents, this
program can cause SIGNIFICANT data loss, and you are advised
to make sure your data is backed up before beginning the
installation process.

Default answers are displayed in brackets after the questions.
You can hit Control-C at any time to quit, but if you do so at a
prompt, you may have to hit return.  Also, quitting in the middle of
installation may leave your system in an inconsistent state.

__welcome_banner_3
}

md_not_going_to_install() {
	cat << \__not_going_to_install_1

OK, then.  Enter `halt' at the prompt to halt the machine.  Once the
machine has halted, power-cycle the system to load new boot code.

__not_going_to_install_1
}

md_congrats() {
	local what;
	if [ "$MODE" = "install" ]; then
		what="installed";
	else
		what="upgraded";
	fi
	cat << __congratulations_1

CONGRATULATIONS!  You have successfully $what NetBSD!
To boot the installed system, enter halt at the command prompt. Once the
system has halted, reset the machine and boot from the disk.

__congratulations_1
}
