#	$NetBSD: Makefile,v 1.12 2002/03/04 16:43:10 shiba Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

.ifndef NOTESOBJDIR
NOTESOBJDIR!=cd ${.CURDIR}/../../../notes/mac68k;${MAKE} ${MAKEFLAGS} -V .OBJDIR
.endif

IMAGE=		ramdisk.fs

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
IMAGE?=		xxx.fs

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	mtree.conf

CRUNCHGEN?=	crunchgen
MTREE?=		mtree

all: ${CBIN} 
	dd if=/dev/zero of=${IMAGE} count=5120
	vnconfig -v -c ${VND_CDEV} ${IMAGE}
	-disklabel ${VND} >tmplabel
	disklabel -r -R ${VND} tmplabel
	newfs -m 0 -o space -i 5120 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} NOTESOBJDIR=${NOTESOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/src/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

release:

clean cleandir distclean:
	if [ -f ${CBIN}.mk ]; then ${MAKE} -f ${CBIN}.mk clean; fi;
	rm -f ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache
	rm -f *.core *.o *.cro *.c tmplabel

.INTERRUPT: unconfig

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
