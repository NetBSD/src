#	$NetBSD: Makefile,v 1.8.2.4 2000/06/03 15:17:46 he Exp $

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
IMAGE?=		ramdisk-${REV}.fs

LISTS=		list
MTREE=		mtree.conf

CYLS=		90
SECS=		18
TRKS=		2
TOTALSECT !=	expr ${CYLS} '*' ${SECS} '*' ${TRKS}
DISKTYPE=	instfs
DISKTAB=	./instfs.disktab
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
	TOPDIR=${TOP} CURDIR=${MINIROOT} REALCURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
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
	awk -f ${MINIROOT}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CBIN}.conf

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

# This is listed in ramdiskbin.conf but is built here.
${CBIN}: libhack.o

# Use stubs to eliminate some large stuff from libc
NOLIBHACKOPENDIR=1
HACKSRC=${TOP}/../utils/libhack
.include "${HACKSRC}/Makefile.inc"

clean cleandir distclean:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache \
	${CBIN}.conf *.o *.lo *.c ${DISKTAB}

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: ${IMAGE}
	gzip -c -9 < $(.OBJDIR)/${IMAGE} \
		> $(RELEASEDIR)/installation/ramdisk/${IMAGE}.gz
.endif	# RELEASEDIR check

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
