#	$NetBSD: Makefile,v 1.12 1999/10/18 20:13:56 pk Exp $

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

SIZE=		1656k
DISKTYPE=	instfs
GEOM=		512/18/2/92
INO_BYTES=	4096

all: ${CBIN}
	dd if=/dev/zero of=${IMAGE} bs=${SIZE} count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	disklabel -W ${VND_CDEV}
	newfs -B be -m 0 -o space -i ${INO_BYTES} -c 92 ${VND_RDEV}
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
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: $(IMAGE)
	gzip -c -9 < $(.OBJDIR)/ramdisk-${REV}.fs \
		> $(RELEASEDIR)/installation/bootfs/ramdisk-${REV}.fs.gz
.endif	# RELEASEDIR check

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
