#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:10 skrll Exp $

DTSSUBDIR=	amlogic
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=		CONFIG_MACH_MESON8=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:10 skrll Exp $

DTSSUBDIR=	amlogic
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=		CONFIG_MACH_MESON8=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
