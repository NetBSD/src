#	$NetBSD: Makefile,v 1.4 2026/10/27 20:06:33 yurix Exp $

# Important: Regenerate dtb set lists by running 'make update-sets' in
# sys/arch/dtb after changing this file.

DTSSUBDIR=	ti/omap
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_OMAP3=y \
		CONFIG_ARCH_OMAP4=y \
		CONFIG_SOC_AM33XX=y
.endif
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# Extra .dts files from arch/arm/dts
.if !empty(MACHINE_ARCH:Mearmv7*)
DTS+=		am335x-ufw.dts
.endif

.include <bsd.dtb.mk>
