#	$NetBSD: Makefile,v 1.1 2002/01/25 15:28:45 reinoud Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGEDIR=	${.OBJDIR}/ramdisk-image
IMAGE=		${.OBJDIR}/ramdisk-${REV}.fs

AUXTARGETS=	install.sh upgrade.sh start.sh
AUXCLEAN=	${AUXTARGETS}

CBIN=		ramdiskbin

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREECONF=	mtree.conf

CRUNCHGEN?=	crunchgen
MTREE?=		mtree

DISKTYPE=	install-ramdisk
RDSIZE=		3800

install.sh: install.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

upgrade.sh: upgrade.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

start.sh: start.tmpl
	sed "s/@@VERSION@@/${VER}/" < ${.ALLSRC} > ${.TARGET}

all: ${AUXTARGETS} ${CBIN} 
	rm -rf ${IMAGEDIR}
	mkdir ${IMAGEDIR}
	${MTREE} -def ${.CURDIR}/${MTREECONF} -p ${IMAGEDIR} -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    KERNOBJDIR=${KERNOBJDIR} \
	    TARGDIR=${IMAGEDIR} sh ${.CURDIR}/runlist.sh ${.CURDIR}/${LISTS}
	${MAKEFS} -B le ${IMAGE} ${IMAGEDIR}
	gzip -c9 ${IMAGE} > ${IMAGE}.gz

unconfig:
	-/bin/rm -rf ${IMAGEDIR} ${IMAGE} ${IMAGE}.gz

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	${CRUNCHGEN} -D ${TOP}/../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk all

clean cleandir distclean:
	/bin/rm -rf ${AUXCLEAN} *.core ${IMAGEDIR}/* ${IMAGE} ${IMAGE}.gz ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
