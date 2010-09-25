#	$NetBSD: install.md,v 1.22 2010/09/25 14:29:13 tsutsui Exp $
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

# Machine-dependent install sets
MDSETS="kern xbase xcomp xfont xserver"

if [ "$MODE" = upgrade ]; then
	RELOCATED_FILES_13="${RELOCATED_FILES_13} /usr/sbin/installboot /usr/mdec/installboot"
fi

md_set_term() {
	if [ ! -z "$TERM" ]; then
		return
	fi
	echo -n "Specify terminal type [vt100]: "
	getresp "vt100"
	TERM="$resp"
	export TERM
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
	dmesg | sed -n -e 's/^\(sd[0-9]\) .*/\1/p' -e 's/^\(x[dy][0-9]\) .*/\1/p' | sort -u
}

md_get_cddevs() {
	# return available CDROM devices
	dmesg | sed -n -e 's/^\(cd[0-9]\) .*/\1/p' | sort -u
}

md_get_ifdevs() {
	# return available network devices
	dmesg | sed -n -e 's/^\(le[0-9]\) .*/\1/p' -e 's/^\(ie[0-9]\) .*/\1/p' | sort -u
}

md_get_partition_range() {
    # return range of valid partition letters
    echo "[a-h]"
}

md_installboot() {
	# $1 is the boot disk
	echo "Installing boot block..."
	cp -p /usr/mdec/boot /mnt/boot
	/usr/sbin/installboot -v /dev/r${1}a /usr/mdec/bootxx /boot
}

md_native_fstype() {
}

md_native_fsopts() {
}

md_checkfordisklabel() {
	# $1 is the disk to check
	local rval
	local cfdl

	cfdl=`disklabel $1 2>&1 > /dev/null | \
	    sed -n -e '/no disk label/{s/.*/ndl/p;q;}; \
		 /disk label corrupted/{s/.*/dlc/p;q;}; \
		 $s/.*/no/p'`
	if [ x$cfdl = xndl ]; then
		rval=1
	elif [ x$cfdl = xdlc ]; then
		rval=2
	else
		rval=0
	fi

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
given in the `sectors/cylinder' entry, which is not shown here).

Do not change any parameters except the partition layout and the label name.
It's probably also wisest not to touch the `8 partitions:' line, even
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
	disklabel -W ${_disk}
	if [ -f /usr/bin/vi ]; then 
		disklabel -e ${_disk}
	else
		disklabel -i ${_disk}
	fi
}

md_copy_kernel() {
	if [ ! -f /mnt/netbsd ]; then
		echo -n "WARNING: No kernel installed; "
		if [ -f /netbsd ]; then
			echo -n "copying miniroot kernel... "
			cp -p /netbsd /mnt/netbsd
			echo "done."
		else
			echo -n "install a kernel manually."
		fi
	fi
}

md_welcome_banner() {
{
	if [ "$MODE" = "install" ]; then
		echo ""
		echo "Welcome to the NetBSD/sparc ${VERSION} installation program."
		cat << \__welcome_banner_1

This program is designed to help you put NetBSD on your disk,
in a simple and rational way.  You'll be asked several questions,
and it would probably be useful to have your disk's hardware
manual, the installation notes, and a calculator handy.
__welcome_banner_1

	else
		echo ""
		echo "Welcome to the NetBSD/sparc ${VERSION} upgrade program."
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
} | more
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

md_lib_is_aout() {
	local r
	test -h $1 && return 1
	test -f $1 || return 1

	[ "`dd if=$1 bs=1 skip=1 count=3 2> /dev/null`" = "ELF" ] && return 1
	return 0
}


md_mv_usr_lib() {
	local root
	root=$1
	for f in $root/usr/lib/lib*.so.[0-9]*.[0-9]* ; do
		md_lib_is_aout $f || continue
		mv -f $f $root/emul/aout/usr/lib || return 1
	done
	return 0
}

md_x_shlib_set_14=" \
	libICE.so.6.3 \
	libPEX5.so.6.0 \
	libSM.so.6.0 \
	libX11.so.6.1 \
	libXIE.so.6.0 \
	libXaw.so.6.1 \
	libXext.so.6.3 \
	libXi.so.6.0 \
	libXmu.so.6.0 \
	libXp.so.6.2 \
	libXt.so.6.0 \
	libXtst.so.6.1 \
	liboldX.so.6.0"

md_mv_x_lib() {
	local root xlibdir
	root=$1
	xlibdir=$2
	for f in $md_x_shlib_set_14; do
		md_lib_is_aout $root/$xlibdir/$f || continue
		mv -f $root/$xlibdir/$f $root/emul/aout/$xlibdir || return 1
	done
	return 0
}

md_mv_aout_libs()
{
	local root xlibdir

	root=/mnt	# XXX - should be global

	if [ -d $root/emul/aout/. ]; then
		echo "Using existing /emul/aout directory"
	else
		echo "Creating /emul/aout hierachy"
		mkdir -p $root/usr/aout || return 1

		if [ ! -d $root/emul ]; then
			mkdir $root/emul || return 1
		fi

		if [ -h $root/emul/aout ]; then
			echo "Preserving existing symbolic link from /emul/aout"
			mv -f $root/emul/aout $root/emul/aout.old || return 1
		fi

		ln -s ../usr/aout $root/emul/aout || return 1
	fi

	# Create /emul/aout/etc and /emul/aout/usr/lib
	if [ ! -d $root/emul/aout/etc ]; then
		mkdir $root/emul/aout/etc || return 1
	fi
	if [ ! -d $root/emul/aout/usr/lib ]; then
		mkdir -p $root/emul/aout/usr/lib || return 1
	fi

	# Move ld.so.conf
	if [ -f $root/etc/ld.so.conf ]; then
		mv -f $root/etc/ld.so.conf $root/emul/aout/etc || return 1
	fi

	# Finally, move the aout shared libraries from /usr/lib
	md_mv_usr_lib $root || return 1

	# If X11 is installed, move the those libraries as well
	xlibdir="/usr/X11R6/lib"
	if [ -d $root/$xlibdir/. ]; then
		mkdir -p $root/emul/aout/$xlibdir || return 1
		md_mv_x_lib $root $xlibdir || return 1
	fi

	echo "a.out emulation environment setup completed."
}

md_prepare_upgrade()  
{
cat << 'EOF'
This release uses the ELF binary object format. Existing (a.out) binaries
can still be used on your system after it has been upgraded, provided
that the shared libraries needed by those binaries are made available
in the filesystem hierarchy rooted at /emul/aout.

This upgrade procedure will now establish this hierarchy by moving all
shared libraries in a.out format found in /usr/lib to /emul/aout/usr/lib.
It will also move the X11 shared libraries in a.out format from previous
NetBSD/sparc X11 installation sets, if they are installed.

EOF
	md_mv_aout_libs || {
		echo "Failed to setup a.out emulation environment"
		return 1
	}
	return 0
}

# Flag to notify upgrade.sh of the existence of md_prepare_upgrade()
md_upgrade_prep_needed=1
