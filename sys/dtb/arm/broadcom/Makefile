#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:10 skrll Exp $

DTSSUBDIR=	broadcom
.if !empty(MACHINE_ARCH:Mearmv6*)
DTSMAKEVARS=	CONFIG_ARCH_BCM2835=y
.elif !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_BCM2835=y
.endif

DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:10 skrll Exp $

DTSSUBDIR=	broadcom
.if !empty(MACHINE_ARCH:Mearmv6*)
DTSMAKEVARS=	CONFIG_ARCH_BCM2835=y
.elif !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_BCM2835=y
.endif

DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
