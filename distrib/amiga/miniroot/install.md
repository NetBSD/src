#	$NetBSD: install.md,v 1.18 2002/04/15 02:48:17 mhitch Exp $
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
MDSETS="kern-GENERIC xbase xcomp xcontrib xfont xmisc xserver"

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
	#
	# Force kern_fs to be mounted
	#
	if [ ! -d /kern -o ! -e /kern/msgbuf ]; then
		mkdir /kern > /dev/null 2>&1
		/sbin/mount_kernfs /kern /kern >/dev/null 2>&1
	fi
}

md_makerootwritable() {
	# Mount root rw for convenience of the tester ;-)
	if [ ! -e /tmp/.root_writable ]; then
		__mount_kernfs
		# XXX: Use /kern/rootdev instead?
		mount -t ffs -u /kern/rootdev / > /dev/null 2>&1
		cp /dev/null /tmp/.root_writable
	fi
}

md_get_diskdevs() {
	# return available disk devices
	__mount_kernfs
	sed -n -e '/^[sw]d[0-9] /s/ .*//p' \
		< /kern/msgbuf | sort -u
}

md_get_cddevs() {
	# return available CDROM devices
	__mount_kernfs
	sed -n -e '/^cd[0-9] /s/ .*//p' \
		< /kern/msgbuf | sort -u
}

md_get_partition_range() {
	# return an expression describing the valid partition id's
	echo '[a-p]'
}

md_installboot() {
	if [ -x /mnt/usr/mdec/installboot ]; then
		echo -n "Should a boot block be installed? [y] "
		getresp "y"
		case "$resp" in
			y*|Y*)
				echo "Installing boot block..."
				chroot /mnt /usr/mdec/installboot /usr/mdec/bootxx_ffs /dev/r${1}a
				cp -p /mnt/usr/mdec/boot.amiga /mnt/
				;;
			*)
				echo "No bootblock installed."
				;;
		esac
	elif [ "$MODE" = "install" ]; then
		cat << \__md_installboot_1
There is no installboot program found on the installed filesystems. No boot
programs are installed.
__md_installboot_1
	else
		cat << \__md_installboot_2
There is no installboot program found on the upgraded filesystems. No boot
programs are installed.
__md_installboot_2
	fi
}

md_native_fstype() {
	echo "ados"
}

md_native_fsopts() {
	echo "ro"
}

md_prep_disklabel() {
	_DKDEVS=`md_get_diskdevs`
	echo "If you like, you can now examine the labels of your disks."
	echo ""
	echo -n "Available are "${_DKDEVS}". Look at which? [skip this step] "
	getresp	"done"
	while [ "X$resp" != "Xdone" ]; do
		echo ""
		disklabel ${resp}
		echo ""
		echo -n "Available are "${_DKDEVS}". Look at which? [done] "
		getresp	"done"
	done
	cat << \__prep_disklabel_1

As a reminder: the 'c' partition is assigned to the whole disk and can't
normally be used for a any file system!

__prep_disklabel_1
}

md_labeldisk() {
}

md_welcome_banner() {
	if [ "$MODE" = "install" ]; then
		echo ""
		echo "Welcome to the NetBSD/amiga ${VERSION} installation program."
		cat << \__welcome_banner_1

This program is designed to help you put NetBSD on your disk,
in a simple and rational way.  You'll be asked several questions,
and it would probably be useful to have your disk's hardware
manual, the installation notes, and a calculator handy.
__welcome_banner_1

	else
		echo ""
		echo "Welcome to the NetBSD/amiga ${VERSION} upgrade program."
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

Note: If you wish to have another try. Just type '^D' at the prompt. After
      a moment, the installer will restart itself.

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

Note: If you wish to have another try. Just type '^D' at the prompt. After
      a moment, the installer will restart itself.

__congratulations_1
}

md_copy_kernel() {
	# This is largely a copy of install_disk and install_from_mounted_fs()
	# with some special frobbing.

	local _directory
	local _sets
	local _filename
	local _f

	if [ "$MODE" = "install" ]; then
		echo -n "Adding keymap initialization to rc.local..."
		echo /usr/sbin/loadkmap ${__keymap__} >> /mnt/etc/rc.local
		echo "done."
	fi

	if [ -e /netbsd ]; then
		if [ -e /mnt/netbsd ]; then
			echo "On the installation filesystem there is this kernel: "
			ls -l /netbsd
			echo "The already installed kernel is: "
			ls -l /mnt/netbsd
			echo	"Do you want to replace the already installed kernel by the kernel"
			echo -n "on the installation filesystem? (y/n) [n] "
			resp="n"
			getresp ""
			if [ "${resp}" != "y" -a "${resp}" != "Y" ]; then
				return
			fi
		fi

		echo -n "Copying kernel..."
		cp -p /netbsd /mnt/netbsd
		echo "done."
		return
	fi

cat << \__md_copy_kernel_1
Your installation set did not include a netbsd kernel on the installation
filesystem. You are now given the opportunity install it from either the
kernel-floppy from the distribution or another location on one of your disks.

The following disk devices are installed on your system; please select
the disk device containing the partition with the netbsd kernel:
__md_copy_kernel_1

	_DKDEVS=`md_get_diskdevs`
	echo    "$_DKDEVS"
	echo	"fd0"
	echo	""
	_DKDEVS="$_DKDEVS fd0"		# Might be on the kernel floppy!
	echo -n	"Which is the disk with the kernel? [abort] "

	if mount_a_disk ; then
		return	# couldn't mount the disk
	fi

	# Get the directory where the file lives
	resp=""		# force one iteration
	while [ "X${resp}" = X"" ]; do
		echo "Enter the directory relative to the mount point that"
		echo -n "contains the file. [${_directory}] "
		getresp "${_directory}"
	done
	_directory=$resp

	_sets=`(cd /mnt2/$_directory; ls netbsd* 2> /dev/null)`
	if [ -z "$_sets" ]; then
		echo "There are no NetBSD kernels available in \"$1\""
		umount -f /mnt2 > /dev/null 2>&1
		return
	fi
	while : ; do
		echo "The following kernels are available:"
		echo ""

		for _f in $_sets ; do
			echo "    $_f"
		done
		echo ""
		set -- $_sets
		echo -n "File name [$1]? "
		getresp "$1"
		_f=$resp
		_filename="/mnt2/$_directory/$_f"

		# Ensure file exists
		if [ ! -f $_filename ]; then
			echo "File $_filename does not exist.  Check to make"
			echo "sure you entered the information properly."
			echo -n "Do you want to retry [y]? "
			getresp "y"
			if [ "$resp" = "n" ]; then
				break
			fi
			continue
		fi

		# Copy the kernel
		cp $_filename /mnt
		break
	done
	umount -f /mnt2 > /dev/null 2>&1
}
