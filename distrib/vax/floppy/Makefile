#	$NetBSD: Makefile,v 1.1.2.2 2001/03/20 18:35:26 he Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

IMAGE=		boot.fs
INSTKERNOBJ!=	cd ${.CURDIR}/../ramdisk-kernel && ${PRINTOBJDIR}
KERN=		${KERNOBJDIR}/INSTALL/netbsd

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_RDEVC=	/dev/r${VND}c
FD?=		fd0
FD_RDEV=	/dev/r${FD}a
MDEC=		${DESTDIR}/usr/mdec

DISKTYPE=	rx23

CLEANFILES+=	boot.fs

dependall all: ${MDEC}/boot ${INSTKERNOBJ}/netbsd.INSTALL.gz
	dd if=/dev/zero of=${IMAGE} bs=1440k count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}
	disklabel -Rr ${VND} ${.CURDIR}/floppy.lbl
	newfs -B le -m 0 -o space -i 56000 -c 80 -f 512 -b 4096 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp ${MDEC}/boot ${MOUNT_POINT}/boot
	cp ${INSTKERNOBJ}/netbsd.INSTALL.gz ${MOUNT_POINT}/netbsd
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	sync
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}
	${MDEC}/installboot -v ${IMAGE} ${MDEC}/xxboot

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}

clean cleandir distclean:
	rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.obj.mk>
