#	$NetBSD: Makefile,v 1.10 2000/07/24 09:27:40 tsubai Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>

IMAGE=		boot.fs
KERN=		${KERNOBJDIR}/INSTALL/netbsd

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
FD?=		fd0
FD_RDEV=	/dev/r${FD}a
MDEC=		${DESTDIR}/usr/mdec

STRIP?=		strip

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk; ${MAKE} print-objdir
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

DISKTYPE=	floppy

CLEANFILES+=	netbsd.ram.gz netbsd.tmp

all: netbsd.ram.gz
	dd if=/dev/zero of=${IMAGE} bs=1440k count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}
	disklabel -f ${DESTDIR}/etc/disktab -rw ${VND} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 65536 -c 80 -f 512 -b 4096 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	cp ${MDEC}/ofwboot ${MOUNT_POINT}/boot
	cp netbsd.ram.gz ${MOUNT_POINT}/netbsd
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	sync
	${MDEC}/installboot -v ${MOUNT_POINT}/boot ${MDEC}/bootxx ${VND_RDEV}
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

clean cleandir distclean:
	rm -f *.core ${IMAGE} ${CLEANFILES}

real-floppy:
	dd if=${IMAGE} of=${FD_RDEV} bs=45k

.include <bsd.obj.mk>
