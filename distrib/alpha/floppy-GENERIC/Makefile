#	$NetBSD: Makefile,v 1.3 1998/11/05 02:47:14 ross Exp $

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
MDEC=		${DESTDIR}/usr/mdec
KERNEL=		${.CURDIR}/../../../sys/arch/alpha/compile/GENERIC/netbsd

GZKERNEL=	netbsd.gz
IMAGE=		GENERIC.fs

DISKTYPE=	floppy

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES?=	6144

CLEANFILES+=	${IMAGE} ${GZKERNEL}

all: GENERIC.fs

GENERIC.fs: ${GZKERNEL} ${MDEC}/boot ${MDEC}/bootxx
	@echo Creating vn image
	dd if=/dev/zero of=${.TARGET}.tmp count=2880
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${.TARGET}.tmp
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	${INSTALL} -c -m 0644 -o ${BINOWN} -g ${BINGRP}	\
		${GZKERNEL} ${MOUNT_POINT}/.
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	cp ${MDEC}/boot ${MOUNT_POINT}
	${MDEC}/installboot -v ${MOUNT_POINT}/boot ${MDEC}/bootxx ${VND_CRDEV}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}
	mv ${.TARGET}.tmp ${.TARGET}

${GZKERNEL}: ${KERNEL}
	gzip -c9 ${KERNEL} > ${.TARGET}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES}

.include <bsd.own.mk>
.include <bsd.obj.mk>
