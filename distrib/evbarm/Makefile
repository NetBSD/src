#	$NetBSD: Makefile,v 1.4 2002/04/13 01:46:29 thorpej Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

SUBDIR=		gzboot instkernel

ITARGET=	${RELEASEDIR}/binary/gzimg

GZIMGDIR!=	cd ${.CURDIR}/gzboot/gzimg && ${PRINTOBJDIR}
GZIMGS!=	cd ${.CURDIR}/gzboot/gzimg && ${MAKE} -V GZIMGS

release: check_RELEASEDIR
.for img in ${GZIMGS}
	${RELINSTALL} ${GZIMGDIR}/${img} ${ITARGET}/.
.endfor

.include <bsd.subdir.mk>
