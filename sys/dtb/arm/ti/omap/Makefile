#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:12 skrll Exp $

DTSSUBDIR=	ti/omap
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_OMAP3=y \
		CONFIG_ARCH_OMAP4=y \
		CONFIG_SOC_AM33XX=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:12 skrll Exp $

DTSSUBDIR=	ti/omap
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_OMAP3=y \
		CONFIG_ARCH_OMAP4=y \
		CONFIG_SOC_AM33XX=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
