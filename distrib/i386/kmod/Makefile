#	$NetBSD: Makefile,v 1.3.2.2 2008/05/18 12:29:36 yamt Exp $

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
		    ${SRCMOD} ${DSTMOD}.tmp
		gzip -9c < ${DSTMOD}.tmp > ${DSTMOD}
		rm -f ${DSTMOD}.tmp

.include <bsd.prog.mk>

release:	miniroot.kmod
		${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG}  \
		    ${RELEASEDIR}/${MACHINE}/installation/miniroot/

