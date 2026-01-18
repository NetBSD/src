#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:10 skrll Exp $

DTSSUBDIR=	allwinner
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=					\
		CONFIG_MACH_SUN4I=y		\
		CONFIG_MACH_SUN5I=y		\
		CONFIG_MACH_SUN6I=y		\
		CONFIG_MACH_SUN7I=y		\
		CONFIG_MACH_SUN8I=y		\
		CONFIG_MACH_SUN9I=y
.endif

DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# Extra .dts files from arch/arm/dts
.if !empty(MACHINE_ARCH:Mearmv7*)
DTS+=		sun5i-r8-pocketchip.dts
DTS+=		sun7i-a20-olinuxino-lime2-emmc-spi.dts
DTS+=		sun8i-h2-plus-bananapi-p2-zero.dts
DTS+=		sun8i-h3-nanopi-r1.dts
.endif

.include <bsd.dtb.mk>
#	$NetBSD: Makefile,v 1.1 2026/01/18 11:34:10 skrll Exp $

DTSSUBDIR=	allwinner
.if !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=					\
		CONFIG_MACH_SUN4I=y		\
		CONFIG_MACH_SUN5I=y		\
		CONFIG_MACH_SUN6I=y		\
		CONFIG_MACH_SUN7I=y		\
		CONFIG_MACH_SUN8I=y		\
		CONFIG_MACH_SUN9I=y
.endif

DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# Extra .dts files from arch/arm/dts
.if !empty(MACHINE_ARCH:Mearmv7*)
DTS+=		sun5i-r8-pocketchip.dts
DTS+=		sun7i-a20-olinuxino-lime2-emmc-spi.dts
DTS+=		sun8i-h2-plus-bananapi-p2-zero.dts
DTS+=		sun8i-h3-nanopi-r1.dts
.endif

.include <bsd.dtb.mk>
