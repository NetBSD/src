#	$NetBSD: Makefile,v 1.2 2021/01/02 11:36:36 ryo Exp $

DTSSUBDIR=	amlogic
DTSMAKEVARS=	CONFIG_ARCH_MESON=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# Extra .dts files from sys/arch/arm/dts
DTS+=		meson-g12b-odroid-n2-plus.dts

.include <bsd.dtb.mk>
