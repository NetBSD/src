#	$NetBSD: Makefile,v 1.16 2001/09/25 06:00:34 jmc Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

CRUNCHGEN?=	crunchgen

SIZE=		2048k
DISKTYPE=	floppyinstfs
DBG=		-Os -mmultiple

all: ${CBIN} 
	dd if=/dev/zero of=${IMAGE} bs=${SIZE} count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}
	disklabel -f ${.CURDIR}/disktab -rw ${VND} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 6144 -c 128 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
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
	${CRUNCHGEN} -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c libhack.o
	${MAKE} -f ${CBIN}.mk DBG="${DBG}" all

HACKSRC=${TOP}/../../utils/libhack
NOLIBHACKOPENDIR=
.include "${HACKSRC}/Makefile.inc"

clean cleandir distclean:
	rm -f ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache
	rm -f *.core *.o *.cro *.c

.include <bsd.sys.mk>
.include <bsd.obj.mk>
