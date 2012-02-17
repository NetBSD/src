#	$NetBSD: Makefile,v 1.11 2012/02/17 00:33:25 riz Exp $

.include <bsd.own.mk>
.include "../../common/Makefile.distrib"

MKMAN=		no
PROG=		miniroot.kmod

SRCMOD=		${DESTDIR}/stand/${MACHINE}/${MODULEVER}/modules/miniroot/miniroot.kmod
DSTMOD=		${.OBJDIR}/miniroot.kmod
RAMDISK=	ramdisk-big
RAMDISKDIR!=	cd ${.CURDIR}/../ramdisks/${RAMDISK} && ${PRINTOBJDIR}
RAMDISKFS=	${RAMDISKDIR}/${RAMDISK}.fs

miniroot.kmod:	${RAMDISKFS} ${SRCMOD}
		${OBJCOPY} --add-section miniroot=${RAMDISKFS} \
		    --set-section-flags miniroot=alloc,contents,load,data \
		    ${SRCMOD} ${DSTMOD}.tmp
		gzip -9nc < ${DSTMOD}.tmp > ${DSTMOD}
		rm -f ${DSTMOD}.tmp

.include <bsd.prog.mk>

release:	miniroot.kmod
		${HOST_INSTALL_FILE} -m ${BINMODE} ${PROG}  \
		    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot/

