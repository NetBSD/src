#	$NetBSD: Makefile,v 1.3 1997/06/23 21:29:50 thorpej Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk-${REV}.fs

AUXTARGETS=	install.sh upgrade.sh start.sh
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
IMAGE?=		xxx-${REV}.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

DISKTYPE=	floppy3

install.sh: install.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

upgrade.sh: upgrade.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

start.sh: start.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

all: ${AUXTARGETS} ${CBIN} 
	dd if=/dev/zero of=${IMAGE} count=2880
	vnconfig -t ${DISKTYPE} -v -c ${VND_DEV} ${IMAGE}
	newfs -m 0 -o space -i 5120 -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}
	cat /*bin/* > /dev/null

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}
	cat /*bin/* > /dev/null

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

clean cleandir:
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.lo *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>

