#	$NetBSD: Makefile.inc,v 1.8 2002/03/28 04:33:25 lukem Exp $

.include "${TOP}/Makefile.inc"

WARNS=1

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
IMAGE?=		xxx.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	mtree.conf

CRUNCHGEN?=	crunchgen
MTREE?=		mtree

realall: ${IMAGE}

RAMDISKSIZE!=	expr ${RAMDISKSPT} \* ${RAMDISKCYLS} \* ${RAMDISKHEADS}
RAMDISKGEOM=	512/${RAMDISKSPT}/${RAMDISKHEADS}/${RAMDISKCYLS}

${IMAGE}: ${CBIN} ${AUXDEPENDS} ${MTREECONF} ${LISTS}
	dd if=/dev/zero of=${IMAGE} count=${RAMDISKSIZE}
	vnconfig -v -c ${VND} ${IMAGE} ${RAMDISKGEOM}
	-disklabel ${VND} >tmplabel
	disklabel -r -R ${VND} tmplabel
	newfs -B be -m 0 -o space -i ${RAMDISKBPI} -c ${RAMDISKCPG} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -U
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} COMMONDIR=${TOP}/ramdisk-common \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CBIN}:	libhack.o

# Used in building bootfloppy
echoimage:
	@echo ${.OBJDIR}/${IMAGE}

depend:

release:

clean cleandir distclean:
	if [ -f ${CBIN}.mk ]; then ${MAKE} -f ${CBIN}.mk clean; fi
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c tmplabel
