#	$NetBSD: Makefile,v 1.5 2002/03/25 07:43:41 lukem Exp $

TOP=		${.CURDIR}/..
WARNS=1

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}d
VND_CRDEV=	/dev/r${VND}d
IMAGE?=		xxx.fs
MDEC=		${DESTDIR}/usr/mdec

#CRUNCHGEN?=	sh-crunchgen

# for cross compile
#CRUNCHIDE?=	sh-crunchide

SUDO?=		sudo
DISKLABEL?=	${SUDO} disklabel
MOUNT?=		${SUDO} mount
MTREE?=		${SUDO} mtree
NEWFS?=		${SUDO} newfs
UMOUNT?=	${SUDO} umount
VNCONFIG?=	${SUDO} vnconfig

LISTS=		../ramdiskcommon/list
CRUNCHCONF=	../ramdiskcommon/${CBIN}.conf
MTREECONF=	../ramdiskcommon/mtree.conf

RAMDISKSPC=	128
RAMDISKCYLS=	54
RAMDISKSIZE!=	expr ${RAMDISKSPC} \* ${RAMDISKCYLS}

realall: ${IMAGE}

${IMAGE}: ${CBIN} ${MTREECONF} ${LISTS}
	rm -f ${IMAGE}.tmp
	dd if=/dev/zero of=${IMAGE}.tmp count=${RAMDISKSIZE}
	${VNCONFIG} -v -c ${VND_CDEV} ${IMAGE}.tmp \
		512/${RAMDISKSPC}/1/${RAMDISKCYLS}
	-${DISKLABEL} ${VND_CDEV} >tmplabel
	${DISKLABEL} -r -R ${VND_CDEV} tmplabel
	${NEWFS} -B be -m 0 -o space ${VND_RDEV} 
	${MOUNT} ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -U
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
		TARGDIR=${MOUNT_POINT} ${SUDO} sh ${TOP}/runlist.sh \
		${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	${UMOUNT} ${MOUNT_POINT}
	${VNCONFIG} -u ${VND_CDEV}
	mv -f ${IMAGE}.tmp ${IMAGE}

unconfig:
	-${UMOUNT} -f ${MOUNT_POINT}
	-${VNCONFIG} -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}.tmp

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../../.. -L ${.CURDIR} ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

# This is listed in stonebin.conf but is built here.
#${CBIN}: libhack.o

# Use stubs to eliminate some large stuff from libc
HACKSRC=${TOP}/../../utils/libhack
.include "${HACKSRC}/Makefile.inc"
HACKOBJS+=	gethost.o

mount:
	${VNCONFIG} -v -c ${VND_CDEV} ${IMAGE} \
		512/${RAMDISKSPC}/1/${RAMDISKCYLS}
	${MOUNT} ${VND_DEV} ${MOUNT_POINT}

clean cleandir distclean:
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${IMAGE}.tmp ${CBIN} \
	    ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c tmplabel

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
