#	$NetBSD: Makefile,v 1.2 2021/06/02 10:31:18 jmcneill Exp $

DTSSUBDIR=	rockchip
DTSMAKEVARS=	CONFIG_ARCH_ROCKCHIP=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# Extra .dts files from sys/arch/arm/dts
DTS+=		rk3399-pinebook-pro.dts

# For compatibility with factory firmware, copy rk3399-pinebook-pro.dtb to
# the base directory.
DTB_NOSUBDIR+=	rk3399-pinebook-pro.dtb

.include <bsd.dtb.mk>
