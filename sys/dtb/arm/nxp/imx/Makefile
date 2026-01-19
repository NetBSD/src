#	$NetBSD: Makefile,v 1.2 2026/01/19 05:25:21 skrll Exp $

DTSSUBDIR=	nxp/imx
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_SOC_IMX6Q=y		\
		CONFIG_SOC_IMX6SX=y		\
		CONFIG_SOC_IMX7D=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# Extra .dts files from arch/arm/dts
.if !empty(MACHINE_ARCH:Mearmv5*)
DTS+=		imx23-olinuxino.dts
.endif

.include <bsd.dtb.mk>
