#!/bin/sh
# $NetBSD: upgrade.sh,v 1.3 2003/07/26 17:06:26 salo Exp $
#
# Copyright (c) 1997 Perry E. Metzger
# Copyright (c) 1994 Christopher G. Demetriou
# All rights reserved.
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
#          This product includes software developed for the
#          NetBSD Project.  See http://www.NetBSD.org/ for
#          information about NetBSD.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>

#	NetBSD upgrade script.
#	In a perfect world, this would be a nice C program, with a reasonable
#	user interface.

DT=/etc/disktab				# /etc/disktab
FSTABDIR=/mnt/etc			# /mnt/etc
#DONTDOIT=echo
UARGS="-c 2"

FSTAB=${FSTABDIR}/fstab

getresp() {
	read resp
	if [ "X$resp" = "X" ]; then
		resp=$1
	fi
}

echo	"Welcome to the original NetBSD/alpha upgrade program."
getresp
echo	"This version of the program has been largely replaced by the new
echo	"sysinst utility. Both programs are on this installation media set.
echo -n "Type the return key to continue..."
echo	""
echo	"This program is designed to help you put the new version of NetBSD"
echo	"on your hard disk, in a simple and rational way.  To upgrade, you"
echo	"must have plenty of free space on all partitions which will be"
echo	"upgraded.  If you have at least 1MB free on your root partition,"
echo	"and several free on your /usr patition, you should be fine."
echo	""
echo	"As with anything which modifies your hard drive's contents, this"
echo	"program can cause SIGNIFICANT data loss, and you are advised"
echo	"to make sure your hard drive is backed up before beginning the"
echo	"upgrade process."
echo	""
echo	"Default answers are displyed in brackets after the questions."
echo	"You can hit Control-C at any time to quit, but if you do so at a"
echo	"prompt, you may have to hit return.  Also, quitting in the middle of"
echo	"the upgrade may leave your system in an inconsistent (and unusable)"
echo	"state."
echo	""
echo -n "Proceed with upgrade? [n] "
getresp "n"
case "$resp" in
	y*|Y*)
		echo	"Cool!  Let's get to it..."
		;;
	*)
		echo	""
		echo	"OK, then.  Enter 'halt' at the prompt to halt the"
		echo	"machine.  Once the machine has halted, remove the"
		echo	"floppy and press any key to reboot."
		exit
		;;
esac

# find out what units are possible, and query the user.
driveunits=`echo /dev/[sw]d?a | sed -e 's,/dev/\(...\)a,\1,g'`
if [ "X${driveunits}" = "X" ]; then
	echo	"FATAL ERROR:"
	echo	"No disk devices."
	echo	"This is probably a bug in the install disks."
	echo	"Exiting install program."
	exit
fi

echo	""
echo	"The following disks are supported by this upgrade procedure:"
echo	"	"${driveunits}
echo	"If your system was previously completely contained within the"
echo	"disks listed above (i.e. if your system didn't occupy any space"
echo	"on disks NOT listed above), this upgrade disk can upgrade your"
echo	"system.  If it cannot, hit Control-C at the prompt."
echo	""
while [ "X${drivename}" = "X" ]; do
	echo -n	"Which disk contains your root partition? "
	getresp
	otherdrives=`echo "${driveunits}" | sed -e s,${resp},,`
	if [ "X${driveunits}" = "X${otherdrives}" ]; then
		echo	""
		echo	"\"${resp}\" is an invalid drive name.  Valid choices"
		echo	"are: "${driveunits}
		echo	""
	else
		drivename=${resp}
	fi
done

echo	""
echo	"Root partition is on ${drivename}a."

echo	""
echo	"If (and only if!) you are upgrading from NetBSD 0.9 or below,"
echo	"you should upgrade to the new file system format. Do not answer"
echo	"yes if you are upgrading from NetBSD 1.0 or above."
echo	"Would you like to upgrade your file systems to the new file system"
echo -n	"format? [y] "
getresp "y"
case "$resp" in
	n*|N*)
		echo	""
		echo	"If you are upgrading from NetBSD 0.9 or below,"
		echo	"you should upgrade your file systems with 'fsck -c 2'"
		echo	"as soon as is feasible, because the new file system"
		echo	"code is better-tested and more performant."
		upgrargs=""
		upgradefs=NO
		;;
	*)
		upgrargs=$UARGS
		upgradefs=YES
		;;
esac

if [ $upgradefs = YES ]; then
	echo	""
	echo	"your file systems will be upgraded while they are checked"
fi

echo	"checking the file system on ${drivename}a..."
	
fsck -f -p $upgrargs /dev/r${drivename}a
if [ $? != 0 ]; then
	echo	"FATAL ERROR: FILE SYSTEM UPGRADE FAILED."
	echo	"You should probably reboot the machine, fsck your"
	echo	"disk(s), and try the upgrade procedure again."
	exit 1
fi

echo	""
echo	"Mounting root partition on /mnt..."
mount /dev/${drivename}a /mnt
if [ $? != 0 ]; then
	echo	"FATAL ERROR: MOUNT FAILED."
	echo	"You should verify that your system is set up as you"
	echo	"described, and re-attempt the upgrade procedure."
	exit 1
fi
echo	"Done."

echo	""
echo -n	"Copying new fsck binary to your hard disk..."
if [ ! -d /mnt/sbin ]; then
	mkdir /mnt/sbin
fi
cp /sbin/fsck /mnt/sbin/fsck
if [ $? != 0 ]; then
	echo	"FATAL ERROR: COPY FAILED."
	echo	"It in unclear why this error would occur.  It looks"
	echo	"like you may end up having to upgrade by hand."
	exit 1
fi
echo	" Done."

echo	""
echo    "Re-mounting root partition read-only..."
mount -u -o ro /dev/${drivename}a /mnt
if [ $? != 0 ]; then
	echo	"FATAL ERROR: RE-MOUNT FAILED."
	echo	"It in unclear why this error would occur.  It looks"
	echo	"like you may end up having to upgrade by hand."
	exit 1
fi
echo	"Done."

echo	""
echo	"checking the rest of your file systems..."
chroot /mnt fsck -f -p $upgrargs
if [ $? != 0 ]; then
	echo	"FATAL ERROR: FILE SYSTEM UPGRADE(S) FAILED."
	echo	"You should probably reboot the machine, fsck your"
	echo	"file system(s), and try the upgrade procedure"
	echo	"again."
	exit 1
fi
	echo	"Done."

echo	""
echo    "Re-mounting root partition read-write..."
mount -u -o rw /dev/${drivename}a /mnt
if [ $? != 0 ]; then
	echo	"FATAL ERROR: RE-MOUNT FAILED."
	echo	"It in unclear why this error would occur.  It looks"
	echo	"like you may end up having to upgrade by hand."
	exit 1
fi
echo	"Done."

echo	""
echo	"Updating boot blocks on ${drivename}..."
# shouldn't be needed, but...
$DONTDOIT rm -f /mnt/boot
$DONTDOIT cp /usr/mdec/boot /mnt/boot
$DONTDOIT /usr/sbin/installboot /dev/r${drivename}c /usr/mdec/bootxx_ffs
if [ $? != 0 ]; then
	echo	"FATAL ERROR: UPDATE OF DISK LABEL FAILED."
	echo	"It in unclear why this error would occur.  It looks"
	echo	"like you may end up having to upgrade by hand."
	exit 1
fi
echo	"Done."

echo	""
echo	"Copying bootstrapping binaries and config files to the hard drive..."
$DONTDOIT cp /mnt/.profile /mnt/.profile.bak
$DONTDOIT pax -s '#^\./etc/.*##' -Xrwpe . /mnt
$DONTDOIT mv /mnt/etc/rc /mnt/etc/rc.bak
$DONTDOIT cp /tmp/.hdprofile /mnt/.profile

echo	""
echo	"Mounting remaining partitions..."
chroot /mnt mount -at ffs > /dev/null 2>&1
echo	"Done."

echo    ""
echo    ""
echo	"OK!  The preliminary work of setting up your disk is now complete,"
echo	"and you can now upgrade the actual NetBSD software."
echo	""
echo	"Right now, your hard disk is mounted on /mnt.  You should consult"
echo	"the installation notes to determine how to load and install the new"
echo	"NetBSD distribution sets, and how to clean up after the upgrade"
echo	"software, when you are done."
echo	""
echo	"GOOD LUCK!"
echo	""
