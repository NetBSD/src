#	$NetBSD: Makefile,v 1.4.2.4 2000/06/03 15:06:42 he Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd1
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

CYLS=		100
SECS=		16
TRKS=		2
TOTALSECT !=	expr ${CYLS} '*' ${SECS} '*' ${TRKS}
DISKTYPE=	floppyinstfs
DISKTAB=	./${DISKTYPE}.disktab
GEOM=		512/${SECS}/${TRKS}/${CYLS}
INO_BYTES=	6144

all: ${CBIN} 
	dd if=/dev/zero of=${IMAGE} bs=512 count=${TOTALSECT}
	vnconfig -v -c ${VND_CDEV} ${IMAGE} ${GEOM}
	echo "${DISKTYPE}:ty=floppy:se#512:nt#${TRKS}:rm#300:ns#${SECS}:nc#${CYLS}:pa#${TOTALSECT}:oa#0:ba#4096:fa#512:ta=4.2BSD:pc#${TOTALSECT}:oc#0:" > ${DISKTAB}
	disklabel -rw -f ${DISKTAB} ${VND_CDEV} ${DISKTYPE}
	disklabel -W ${VND_CDEV}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c ${CYLS} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
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
	crunchgen -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c libhack.o
	make -f ${CBIN}.mk all

NOLIBHACKOPENDIR=1
HACKSRC=${TOP}/../../utils/libhack
.include "${HACKSRC}/Makefile.inc"
# turn off small gethostby* temporarily
HACKOBJS:= getcap.o getgrent.o getnet.o getnetgr.o getpwent.o setlocale.o yplib.o

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.lo *.c

release:

.include <bsd.obj.mk>
.include <bsd.subdir.mk>

