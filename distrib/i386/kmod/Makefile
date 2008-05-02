#	$NetBSD: Makefile,v 1.1 2008/05/02 23:13:06 ad Exp $

.include <bsd.own.mk>

MKMAN=		no
PROG=		miniroot.kmod

SRCMOD=		${DESTDIR}/kernel/modules/miniroot/miniroot.kmod
DSTMOD=		${.OBJDIR}/miniroot.kmod
RAMDISK=	ramdisk-big
RAMDISKDIR!=	cd ${.CURDIR}/../ramdisks/${RAMDISK} && ${PRINTOBJDIR}
RAMDISKFS=	${RAMDISKDIR}/${RAMDISK}.fs

miniroot.kmod:	${RAMDISKFS} ${SRCMOD}
		${OBJCOPY} --add-section miniroot=${RAMDISKFS} \
		    --set-section-flags miniroot=alloc,contents,load,data \
		    ${SRCMOD} ${DSTMOD}

.include <bsd.prog.mk>

release:	miniroot.kmod

