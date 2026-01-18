#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:11 skrll Exp $

DTSSUBDIR=	rockchip
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_ROCKCHIP=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:11 skrll Exp $

DTSSUBDIR=	rockchip
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_ROCKCHIP=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
