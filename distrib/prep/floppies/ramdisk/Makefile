#	$NetBSD: Makefile,v 1.3 2001/10/13 06:15:20 jmc Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

AUXDEPENDS=	disktab.preinstall dot.profile termcap.src

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RCDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	mtree.conf

CRUNCHGEN?=	crunchgen
MTREE?=		mtree

SIZE=           2048k
DISKTYPE=	floppyinstfs
DBG=            -Os -mmultiple

all: ${IMAGE}

${IMAGE}: ${CBIN} ${AUXDEPENDS} ${MTREECONF} ${LISTS}
	rm -f ${IMAGE}.tmp
	dd if=/dev/zero of=${IMAGE}.tmp bs=${SIZE} count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}.tmp
	disklabel -f ${.CURDIR}/disktab -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 6144 -c 128 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}
	mv -f ${IMAGE}.tmp ${IMAGE}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c libhack.o
	make -f ${CBIN}.mk DBG="${DBG}" all

HACKSRC=	${TOP}/../../utils/libhack
.include "${HACKSRC}/Makefile.inc"
HACKOBJS=	gethost.o getnet.o getpwent.o localeconv.o perror.o \
		setlocale.o strerror.o strsignal.o utmp.o yplib.o

release:

clean cleandir distclean:
	rm -f ${IMAGE} ${IMAGE}.tmp ${CBIN} ${CBIN}.mk ${CBIN}.cache
	rm -f *.core *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
