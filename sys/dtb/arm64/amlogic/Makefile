#	$NetBSD: Makefile,v 1.1.2.1 2021/01/03 16:35:03 thorpej Exp $

DTSSUBDIR=	amlogic
DTSMAKEVARS=	CONFIG_ARCH_MESON=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# Extra .dts files from sys/arch/arm/dts
DTS+=		meson-g12b-odroid-n2-plus.dts

.include <bsd.dtb.mk>
