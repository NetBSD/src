#	$NetBSD: Makefile,v 1.3 2002/03/28 04:33:23 lukem Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

AUXDEPENDS=	dot.profile disktab.preinstall termcap.src

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	mtree.conf

CRUNCHGEN?=	crunchgen
MTREE?=		mtree

DISKSIZE=	2048k
DISKTYPE=	floppyinstfs
DBG=		-Os -mmultiple

realall: ${IMAGE}

${IMAGE}: ${CBIN}  ${AUXDEPENDS} ${MTREECONF} ${LISTS}
	dd if=/dev/zero of=${IMAGE} bs=${DISKSIZE} count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND} ${IMAGE}
	disklabel -f ${.CURDIR}/disktab -rw ${VND} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 6144 -c 128 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -u
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

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk DBG="${DBG}" all

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CBIN}:	libhack.o

release:

clean cleandir distclean:
	if [ -f ${CBIN}.mk ]; then ${MAKE} -f ${CBIN}.mk clean; fi;
	rm -f ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache
	rm -f *.core *.o *.cro *.c

.include <bsd.sys.mk>
.include <bsd.obj.mk>
.include <bsd.prog.mk>
