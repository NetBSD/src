#	$NetBSD: Makefile,v 1.13 2001/04/09 20:23:00 tsubai Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>

IMAGE=		boot.fs

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_RDEVC=	/dev/r${VND}c
FD?=		fd0
FD_RDEV=	/dev/r${FD}a
MDEC=		${DESTDIR}/usr/mdec

KERNDIR!=	cd ${.CURDIR}/../md-kernel; ${MAKE} print-objdir

DISKTYPE=	floppy

dependall all:
	dd if=/dev/zero of=${IMAGE} bs=1440k count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}
	disklabel -f ${DESTDIR}/etc/disktab -rw ${VND} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 65536 -c 80 -f 512 -b 4096 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp ${MDEC}/ofwboot ${MOUNT_POINT}/boot
	cp ${KERNDIR}/netbsd.INSTALL.gz ${MOUNT_POINT}/netbsd
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	sync
	${MDEC}/installboot -v ${MOUNT_POINT}/boot ${MDEC}/bootxx ${VND_RDEVC}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}

clean cleandir distclean:
	rm -f *.core ${IMAGE}

real-floppy:
	dd if=${IMAGE} of=${FD_RDEV} bs=45k

.include <bsd.obj.mk>
