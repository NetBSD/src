#	$NetBSD: Makefile,v 1.3.4.1 1999/12/27 18:28:24 wrstuden Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		boot.fs
KERN=		${.CURDIR}/../../../../sys/arch/macppc/compile/INSTALL/netbsd

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
FD?=		fd0
FD_RDEV=	/dev/r${FD}a
IMAGE?=		xxx.fs
KERN?=		netbsd-xxx
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${.CURDIR}/list
RAMDISK!=	cd $(.CURDIR)/../ramdisk/; \
		    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
		    ${MAKE} -s -f-

DISKTYPE=	floppy

# Some reasonable values for the -i parameter to newfs are:
#
#  8192		1387k, 189 inodes free
# 16384		1399k,  93 inodes free
# 65536		1407k,  29 inodes free
INO_BYTES?=	65536

CLEANFILES+=	netbsd.ram.gz netbsd.tmp

all:	netbsd.ram.gz
	dd if=/dev/zero of=${IMAGE} bs=1440k count=1
	# vnconfig -v -c ${VND_CDEV} ${IMAGE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -f ${DESTDIR}/etc/disktab -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 80 \
	    -f 1024 -b 8192 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	cp ${MDEC}/ofwboot ${MOUNT_POINT}/boot
	sync
	${MDEC}/installboot -v ${MOUNT_POINT}/boot ${MDEC}/bootxx ${VND_RDEV}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

real-floppy:
	dd if=${IMAGE} of=${FD_RDEV} bs=45k

.include <bsd.obj.mk>
