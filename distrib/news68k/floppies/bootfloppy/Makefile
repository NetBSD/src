#	$NetBSD: Makefile,v 1.5 2000/06/14 10:24:24 tsutsui Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=		boot.fs
.if !make(print-objdir)
KERNDIR!=	cd ${.CURDIR}/../ramdisk-kernel && ${MAKE} print-objdir
.endif
KERN=		${KERNDIR}/netbsd.INSTALL.gz

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

DISKTYPE=	floppy

# Some reasonable values for the -i parameter to newfs are:
#
#  8192		1387k, 189 inodes free
# 16384		1399k,  93 inodes free
# 65536		1407k,  29 inodes free
INO_BYTES?=	65536

all:	${KERN}
	dd if=/dev/zero of=${IMAGE} bs=1440k count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 80 \
	    -f 1024 -b 8192 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp ${KERN} ${MOUNT_POINT}/netbsd
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	cp ${MDEC}/boot ${MOUNT_POINT}/boot
	sync
	${MDEC}/installboot -v ${MOUNT_POINT}/boot ${MDEC}/bootxx ${VND_CRDEV}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}

.ifndef RELEASEDIR
release:
	@echo set RELEASEDIR first!
	@false
.else
release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	gzip -c -9 ${IMAGE} > ${RELEASEDIR}/installation/floppy/${IMAGE}.gz
.endif

clean cleandir distclean:
	rm -f *.core ${IMAGE} ${CLEANFILES}

real-floppy:
	dd if=${IMAGE} of=${FD_RDEV} bs=45k

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
