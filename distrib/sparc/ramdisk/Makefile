#	$NetBSD: Makefile,v 1.18.4.3 2002/03/25 17:33:03 he Exp $
#
# ramdisk.fs is the microroot filesystem intended for use with
# the INSTALL kernel. It provides just enough tools to extract the
# miniroot-based `instfs' filesystem into an MFS mount point
# and start the installation scripts from there (see dot.profile).
#

TOP=		${.CURDIR}/..
MINIROOT=	${.CURDIR}/../../miniroot

.include "${TOP}/Makefile.inc"

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
MDEC=		${DESTDIR}/usr/mdec
IMAGE?=		ramdisk.fs

LISTS=		list
MTREE=		mtree.conf

# image size in 512-byte blocks
SIZE=		1360
#DISKTYPE=	instfs
GEOM=		512/1/${SIZE}/1
INO_BYTES=	2048

all: ${CBIN}
	dd if=/dev/zero of=${IMAGE} bs=${SIZE}b count=1
	#vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	vnconfig -v -c ${VND_CDEV} ${IMAGE} ${GEOM}
	#disklabel -rw ${VND_CDEV} ${DISKTYPE}
	#disklabel -W ${VND_CDEV}
	newfs -B be -m 0 -o space -i ${INO_BYTES} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${MINIROOT} REALCURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${MINIROOT}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CBIN}.conf
	crunchgen -D ${TOP}/../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}.conf: ${LISTS}
	awk -f ${MINIROOT}/makeconf.awk CBIN=${CBIN} ${.CURDIR}/${LISTS} > ${CBIN}.conf

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c ${VERDEP}
	make -f ${CBIN}.mk all

# This is listed in ramdiskbin.conf but is built here.
${CBIN}: libhack.o

# Use stubs to eliminate some large stuff from libc
NOLIBHACKOPENDIR=1
HACKSRC=${TOP}/../utils/libhack
.include "${HACKSRC}/Makefile.inc"

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	${CBIN}.conf *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
