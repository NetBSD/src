#	$NetBSD: Makefile,v 1.2.2.2 1999/10/20 23:09:53 he Exp $

BINDIR=		/installation/netboot

TOP?=		${.CURDIR}/..
ARCHDIR?=        ${.CURDIR}/../../${MACHINE}/miniroot

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs
DISKIMAGE=	diskimage.tgz

CBIN=		ramdiskbin
#CSIZE?=	6144
CSIZE?=		4096
MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
MDEC=		${DESTDIR}/usr/mdec

CLEANFILES=	${IMAGE} ${DISKIMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

all: ${IMAGE}

${IMAGE}: ${CBIN} 
	dd if=/dev/zero of=${IMAGE} count=${CSIZE}
	vnconfig -v -c ${VND_CDEV} ${IMAGE}
#	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	-(disklabel ${VND_CDEV} > /tmp/proto; disklabel -R -r ${VND_CDEV} /tmp/proto)
	-newfs -B le -m 0 -o space -i 5120 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} ARCHDIR=${ARCHDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/src/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo Building diskimage tar file...
	tar cf - -C ${MOUNT_POINT} . | gzip -9 > ${DISKIMAGE}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${TOP}/../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

realinstall:
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o root -g wheel \
		-m ${NONBINMODE} ${DISKIMAGE} ${RELEASEDIR}${BINDIR}

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} *.o *.cro *.c

# XXX!
depend:

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include "../Makefile.inc"
