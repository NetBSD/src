#	$NetBSD: Makefile,v 1.21 2002/03/06 23:50:57 lukem Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk-${DISTRIBREV}.fs

AUXTARGETS=	install.sh upgrade.sh start.sh
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
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

DISKTYPE=	install-ramdisk
RDSIZE=		3800

install.sh: install.tmpl
	sed "s/@@VERSION@@/${DISTRIBVER}/" < ${.ALLSRC} > ${.TARGET}

upgrade.sh: upgrade.tmpl
	sed "s/@@VERSION@@/${DISTRIBVER}/" < ${.ALLSRC} > ${.TARGET}

start.sh: start.tmpl
	sed "s/@@VERSION@@/${DISTRIBVER}/" < ${.ALLSRC} > ${.TARGET}

all: ${AUXTARGETS} ${CBIN} 
	dd if=/dev/zero of=${IMAGE} count=${RDSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CRDEV} ${DISKTYPE}
	newfs -B le -m 0 -o space -i 5152 -c 104 -s${RDSIZE} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	${MTREE} -def ${.CURDIR}/${MTREECONF} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${.CURDIR}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}
	gzip -c9 ${IMAGE} > ${IMAGE}.gz

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}
	-/bin/rm -f ${IMAGE} ${IMAGE}.gz

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

clean cleandir distclean:
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${IMAGE}.gz ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
