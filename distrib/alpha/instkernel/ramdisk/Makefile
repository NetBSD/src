#	$NetBSD: Makefile,v 1.11 2001/10/10 05:13:51 enami Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
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
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	mtree.conf

CRUNCHGEN?=	crunchgen

all: ${CBIN} 
	dd if=/dev/zero of=${IMAGE} count=9000
	vnconfig -v -c ${VND_CDEV} ${IMAGE}
#	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B le -m 0 -o space -i 5120 -c 20 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/src/runlist.sh ${.CURDIR}/${LISTS}
	@echo chdir '$${.CURDIR}/../../../notes/alpha; ${MAKE} INSTALL.more'
	@chdir ${.CURDIR}/../../../notes/alpha; ${MAKE} INSTALL.more
	cp `chdir ${.CURDIR}/../../../notes/alpha; ${MAKE} echomore`	\
		${MOUNT_POINT}
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

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
