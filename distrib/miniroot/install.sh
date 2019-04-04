#!/bin/sh
#	$NetBSD: install.sh,v 1.27 2019/04/04 20:51:35 christos Exp $
#
# Copyright (c) 1996,1997,1999,2000,2006 The NetBSD Foundation, Inc.
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

FILESYSTEMS="/tmp/filesystems"		# used thoughout
FQDN=""					# domain name

trap "umount /tmp > /dev/null 2>&1" 0

MODE="install"

# include machine-dependent functions
# The following functions must be provided:
#	md_copy_kernel()	- copy a kernel to the installed disk
#	md_get_diskdevs()	- return available disk devices
#	md_get_cddevs()		- return available CD-ROM devices
#	md_get_ifdevs()		- return available network interfaces
#	md_get_partition_range() - return range of valid partition letters
#	md_installboot()	- install boot-blocks on disk
#	md_labeldisk()		- put label on a disk
#	md_prep_disklabel()	- label the root disk
#	md_welcome_banner()	- display friendly message
#	md_not_going_to_install() - display friendly message
#	md_congrats()		- display friendly message
#	md_native_fstype()	- native filesystem type for disk installs
#	md_native_fsopts()	- native filesystem options for disk installs
#	md_makerootwritable()	- make root writable (at least /tmp)

# The following are optional:
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
THESETS="$ALLSETS $MDSETS"

# Good {morning,afternoon,evening,night}.
md_welcome_banner
echo -n "Proceed with installation? [n] "
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

# Get timezone info
get_timezone

# Make sure we can write files (at least in /tmp)
# This might make an MFS mount on /tmp, or it may
# just re-mount the root with read-write enabled.
md_makerootwritable

# Create a disktab file; lets us write to it for temporary
# purposes without mounting the miniroot read-write.
echo "# disktab" > /tmp/disktab.shadow

test "$md_view_labels_possible" && md_view_labels

while [ -z "${ROOTDISK}" ]; do
	getrootdisk
done

# Deal with disklabels, including editing the root disklabel
# and labeling additional disks.  This is machine-dependent since
# some platforms may not be able to provide this functionality.
md_prep_disklabel ${ROOTDISK}

# Assume partition 'a' of $ROOTDISK is for the root filesystem.  Loop and
# get the rest.
# XXX ASSUMES THAT THE USER DOESN'T PROVIDE BOGUS INPUT.
cat << \__get_filesystems_1

You will now have the opportunity to enter filesystem information.
You will be prompted for device name and mount point (full path,
including the prepending '/' character).

Note that these do not have to be in any particular order.  You will
be given the opportunity to edit the resulting 'fstab' file before
any of the filesystems are mounted.  At that time you will be able
to resolve any filesystem order dependencies.

__get_filesystems_1

echo	"The following will be used for the root filesystem:"
echo	"	${ROOTDISK}a	/"

echo	"${ROOTDISK}a	/" > ${FILESYSTEMS}

resp="not-done"	# force at least one iteration
while [ "$resp" != "done" ]; do
	echo	""
	echo -n	"Device name? [RETURN if you already entered all devices] "
	getresp "done"
	case "$resp" in
	done)
		;;

	*)
		_device_name=$(basename $resp)

		# force at least one iteration
		_first_char="X"
		while [ "${_first_char}" != "/" ]; do
			echo -n "Mount point? "
			getresp ""
			_mount_point=$resp
			if [ "${_mount_point}" = "/" ]; then
				# Invalid response; no multiple roots
				_first_char="X"
			else
				_first_char=$(firstchar ${_mount_point})
			fi
		done
		echo "${_device_name}	${_mount_point}" >> ${FILESYSTEMS}
		resp="X"	# force loop to repeat
		;;
	esac
done

# configure swap
resp=""		# force at least one iteration
while [ -z "${resp}" ]; do
	echo -n	"Ok to configure ${ROOTDISK}b as a swap device? [] "
	getresp ""
	case "$resp" in
	y*|Y*)
		echo "${ROOTDISK}b	swap" >> ${FILESYSTEMS}
		;;
	n*|N*)
		;;
	*)	;;
	esac
done


echo	""
echo	"You have configured the following devices and mount points:"
echo	""
cat ${FILESYSTEMS}
echo	""
echo	"Filesystems will now be created on these devices.  If you made any"
echo -n	"mistakes, you may edit this now.  Edit? [n] "
getresp "n"
case "$resp" in
	y*|Y*)
		${EDITOR} ${FILESYSTEMS}
		;;
	*)
		;;
esac

# Loop though the file, place filesystems on each device.
echo	"Creating filesystems..."
(
	while read _device_name _mp; do
		if [ "$_mp" != "swap" ]; then
			newfs /dev/r${_device_name}
			echo ""
		fi
	done
) < ${FILESYSTEMS}

# Get network configuration information, and store it for placement in the
# root filesystem later.
cat << \__network_config_1
You will now be given the opportunity to configure the network.  This will
be useful if you need to transfer the installation sets via FTP or NFS.
Even if you choose not to transfer installation sets that way, this
information will be preserved and copied into the new root filesystem.

Note, enter all symbolic host names WITHOUT the domain name appended.
I.e. use 'hostname' NOT 'hostname.domain.name'.

__network_config_1
echo -n	"Configure the network? [y] "
getresp "y"
case "$resp" in
	y*|Y*)
		resp=""		# force at least one iteration
		if [ -f /etc/myname ]; then
			resp=$(cat /etc/myname)
		fi
		echo -n "Enter system hostname: [$resp] "
		while [ -z "${resp}" ]; do
			getresp "$resp"
		done
		hostname $resp
		echo $resp > /tmp/myname

		echo -n "Enter DNS domain name: "
		getresp "none"
		if [ "${resp:-none}" != "none" ]; then
			FQDN=$resp
		fi

		configurenetwork

		echo -n "Enter IP address of default route: [none] "
		getresp "none"
		if [ "${resp:-none}" != "none" ]; then
			route delete default > /dev/null 2>&1
			if route add default $resp > /dev/null ; then
				echo $resp > /tmp/mygate
			fi
		fi

		resp="none"
		if [ -n "${FQDN}" ]; then
			echo -n	"Enter IP address of primary nameserver: [none] "
			getresp "none"
		fi
		if [ "${resp:-none}" != "none" ]; then
			echo "domain $FQDN" > /tmp/resolv.conf
			echo "nameserver $resp" >> /tmp/resolv.conf
			echo "search $FQDN" >> /tmp/resolv.conf

			echo -n "Would you like to use the nameserver now? [y] "
			getresp "y"
			case "$resp" in
				y*|Y*)
					cp /tmp/resolv.conf \
					    /tmp/resolv.conf.shadow
					;;

				*)
					;;
			esac
		fi

		echo ""
		echo "The host table is as follows:"
		echo ""
		cat /tmp/hosts
		echo ""
		echo "You may want to edit the host table in the event that"
		echo "you need to mount an NFS server."
		echo -n "Would you like to edit the host table? [n] "
		getresp "n"
		case "$resp" in
			y*|Y*)
				${EDITOR} /tmp/hosts
				;;

			*)
				;;
		esac

		cat << \__network_config_2

You will now be given the opportunity to escape to the command shell to
do any additional network configuration you may need.  This may include
adding additional routes, if needed.  In addition, you might take this
opportunity to redo the default route in the event that it failed above.
If you do change the default route, and wish for that change to carry over
to the installed system, execute the following command at the shell
prompt:

	echo <ip_address_of_gateway> > /tmp/mygate

where <ip_address_of_gateway> is the IP address of the default router.

__network_config_2
		echo -n "Escape to shell? [n] "
		getresp "n"
		case "$resp" in
			y*|Y*)
				echo "Type 'exit' to return to install."
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
# fstab.
(
	while read _dev _mp; do
		if [ "$_mp" = "/" ]; then
			echo /dev/$_dev $_mp ffs rw 1 1
		elif [ "$_mp" = "swap" ]; then
			echo /dev/$_dev none swap sw 0 0
		else
			echo /dev/$_dev $_mp ffs rw 1 2
		fi
	done
) < ${FILESYSTEMS} > /tmp/fstab

echo	"The fstab is configured as follows:"
echo	""
cat /tmp/fstab
cat << \__fstab_config_1

You may wish to edit the fstab.  For example, you may need to resolve
dependencies in the order which the filesystems are mounted.  You may
also wish to take this opportunity to place NFS mounts in the fstab.
This would be especially useful if you plan to keep '/usr' on an NFS
server.

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

echo ""
munge_fstab /tmp/fstab /tmp/fstab.shadow
mount_fs /tmp/fstab.shadow

mount | while read line; do
	set -- $line
	if [ "$2" = "/" ] && [ "$3" = "nfs" ]; then
		echo "You appear to be running diskless."
		echo -n	"Are the install sets on one of your currently mounted filesystems? [n] "
		getresp "n"
		case "$resp" in
			y*|Y*)
				get_localdir
				;;
			*)
				;;
		esac
	fi
done

install_sets

# Copy in configuration information and make devices in target root.
(
	cd /tmp
	for file in fstab ifconfig.* hosts myname mygate resolv.conf; do
		if [ -f $file ]; then
			echo -n "Copying $file..."
			cp $file /mnt/etc/$file
			echo "done."
		fi
	done

	# Enable rc.conf
	if [ -e /mnt/etc/rc.conf ]; then
		cp /mnt/etc/rc.conf /tmp
		sed 's/^rc_configured=NO/rc_configured=YES/' /tmp/rc.conf \
			> /mnt/etc/rc.conf
	fi

	# If no zoneinfo on the installfs, give them a second chance
	if [ ! -e /usr/share/zoneinfo ]; then
		get_timezone
	fi
	if [ ! -e /mnt/usr/share/zoneinfo ]; then
		echo "Cannot install timezone link..."
	else
		echo -n "Installing timezone link..."
		rm -f /mnt/etc/localtime
		ln -s /usr/share/zoneinfo/$TZ /mnt/etc/localtime
		echo "done."
	fi
	if [ ! -x /mnt/dev/MAKEDEV ]; then
		echo "No /dev/MAKEDEV installed, something is wrong here..."
	else
		echo -n "Making devices..."
		pid=$(twiddle)
		cd /mnt/dev
		sh MAKEDEV all
		kill $pid
		echo "done."
	fi
	md_copy_kernel

	md_installboot ${ROOTDISK}
)

unmount_fs /tmp/fstab.shadow

# Pat on the back.
md_congrats

# ALL DONE!
exit 0
