#	$NetBSD: Makefile.inc,v 1.1.2.4 2002/03/25 17:33:06 he Exp $

.include "${TOP}/Makefile.inc"

WARNS=1

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
MTREE=		mtree.conf

CRUNCHGEN?=	crunchgen

realall: ${IMAGE}

RAMDISKSIZE!=	expr ${RAMDISKSPT} \* ${RAMDISKCYLS} \* ${RAMDISKHEADS}
RAMDISKGEOM=	512/${RAMDISKSPT}/${RAMDISKHEADS}/${RAMDISKCYLS}

${IMAGE}: ${CBIN} ${AUXDEPENDS} ${MTREE} ${LISTS}
	dd if=/dev/zero of=${IMAGE} count=${RAMDISKSIZE}
	vnconfig -v -c ${VND_CDEV} ${IMAGE} ${RAMDISKGEOM}
	newfs -B be -m 0 -o space -i ${RAMDISKBPI} -c ${RAMDISKCPG} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -U
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} COMMONDIR=${TOP}/ramdisk-common \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
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

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c ${VERDEP}
	${MAKE} -f ${CBIN}.mk all

# Use size-reduced libhack.
${CBIN}: libhack.o

HACKSRC=${TOP}/../../utils/libhack
.include "${HACKSRC}/Makefile.inc"

# turn off small gethostby* temporarily
HACKOBJS:= getcap.o getgrent.o getnet.o getnetgr.o getpwent.o localeconv.o perror.o setlocale.o strerror.o strsignal.o utmp.o yplib.o
DBG=	-Os

# Used in building bootfloppy
echoimage:
	@echo ${.OBJDIR}/${IMAGE}

depend:

release:

clean cleandir distclean:
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c
