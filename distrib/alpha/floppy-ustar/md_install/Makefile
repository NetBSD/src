#	$NetBSD: Makefile,v 1.3 1998/10/05 04:33:12 ross Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		md_install-${REV}.fs
CBIN=		md_install

AUXTARGETS=	install.sh upgrade.sh start.sh
AUXCLEAN=	${AUXTARGETS}

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
IMAGE?=		xxx-${REV}.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
MTREE=		mtree.conf

DISKTYPE=	floppy

CRUNCHCONF=	${CBIN}.conf
CRUNCHFILES=	${CBIN}.mk ${CBIN}.cache ${CBIN}.c
CRUNCHCMD=	crunchgen -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

install.sh: install.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

upgrade.sh: upgrade.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

start.sh: start.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

all: ${AUXTARGETS} ${CBIN} 
	dd if=/dev/zero of=${IMAGE} count=4096
	vnconfig -v -c ${VND_CDEV} ${IMAGE}
	#vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	#disklabel -rw ${VND_CDEV} ${DISKTYPE}
	#newfs -m 0 -o space -i 5120 -c 80 ${VND_RDEV} ${DISKTYPE}
	newfs -m 0 -o space -i 5120 ${VND_RDEV}
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


${CRUNCHFILES}:	${CRUNCHCONF}
	if ${CRUNCHCMD}; then exit 0; else rm -f ${CRUNCHFILES}; exit 1; fi

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c yplib.o
	make -f ${CBIN}.mk all

clean cleandir distclean:
	/bin/rm -f ${AUXCLEAN} *.core ${IMAGE} ${CBIN} ${CBIN}.mk	\
		${CBIN}.cache *.o *.lo *.c .tmp_md_install*

# We don't build the real libhack, since we don't actually want to kill the
# resolver. But we do want to kill all the yp_ and xdr_ stuff, so extract
# yplib.o from libhack.

HACKSRC=${TOP}/../../utils/libhack
.include "${HACKSRC}/Makefile.inc"

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
