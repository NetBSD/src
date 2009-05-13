#!/bin/sh
# $NetBSD: build_image.sh,v 1.1.1.1.42.1 2009/05/13 19:17:39 jym Exp $
# This script is a quick hack to generate the various floppies in the
# 'installation/floppies' directory. This script cannot be run in the
# build environment, it is provided as a howto.
# When msdos support is added to makefs, it makes sense to provide a
# decent script that can be integrated into the build environment.
#
# Leo 10 Sept. 2002.

DESTDIR=/tmp/flop_images
TOOLDIR=/tmp/flop_tools
KERNEL_DIR=/tmp/kernels
MNT_DIR=/mnt2
VND_DEV=vnd0
SUDO=sudo

IMAGE720="boot BOOT"
IMAGE144="hades_boot HADES milan_isa_boot MILAN-ISAIDE"
IMAGE144="$IMAGE144  milan_pci_boot MILAN-PCIIDE"
TOOLS="chg_pid.ttp file2swp.ttp loadbsd.ttp rawwrite.ttp aptck.ttp"

unpack_tools() {
	if [ ! -d $TOOLDIR ]; then
		mkdir $TOOLDIR;
	fi
	for i in $TOOLS
	do
		cat ${i}.gz.uu | (cd $TOOLDIR; uudecode)
		gunzip -f $TOOLDIR/${i}.gz
	done
}

do_images() {
	base_img=$1; geom=$2; shift 2

	while : ; do
		if [ -z "$1" ]; then
			break;
		fi
		cat ${base_img}.fs.gz.uu | (cd /tmp; uudecode)
		gunzip /tmp/${base_img}.fs.gz
		$SUDO vnconfig $VND_DEV /tmp/${base_img}.fs $geom
		$SUDO mount -t msdos /dev/${VND_DEV}c ${MNT_DIR}

		# Copy the kernel first...
		cp ${KERNEL_DIR}/netbsd-${2}.gz ${MNT_DIR}/netbsd

		# Thereafter the tools, some may not fit :-(
		for i in $TOOLS; do
			cp $TOOLDIR/$i ${MNT_DIR} 2> /dev/null
			if [ $? -ne 0 ]; then
				echo "$i does not fit on ${1}.fs"
				rm -f ${MNT_DIR}/$i
			fi
		done
		echo "Contents of ${1}.fs:\n"; ls -l ${MNT_DIR}

		$SUDO umount ${MNT_DIR}
		$SUDO vnconfig -u ${VND_DEV}
		mv /tmp/${base_img}.fs /tmp/$1.fs
		gzip -9n /tmp/$1.fs
		mv /tmp/$1.fs.gz $DESTDIR
		shift 2
	done
}


if [ ! -d $DESTDIR ]; then
	mkdir $DESTDIR
fi

if [ ! -d $KERNEL_DIR ]; then
	echo "Please put the kernel images in $KERNEL_DIR!!"
	exit 1
fi
rm -f $TOOLDIR/* $DESTDIR/*

unpack_tools
do_images boot720 "512/18/1/80" ${IMAGE720}
do_images boot144 "512/18/2/80" ${IMAGE144}

echo "The images can be found in: $DESTDIR"
