#	$NetBSD: Makefile,v 1.9.2.2 1999/11/08 23:51:31 mycroft Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
VND?=		vnd1
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

SIZE=		2048k
DISKTYPE=	floppyinstfs

all: ${CBIN} 
	dd if=/dev/zero of=${IMAGE} bs=${SIZE} count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}
	disklabel -f ${.CURDIR}/disktab -rw ${VND} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 6144 -c 128 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c #libhack.o
	make -f ${CBIN}.mk all

#HACKSRC=${TOP}/../../utils/libhack
#.include "${HACKSRC}/Makefile.inc"

clean cleandir distclean:
	rm -f ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache
	rm -f *.core *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
