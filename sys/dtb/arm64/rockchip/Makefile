#	$NetBSD: Makefile,v 1.3 2021/11/07 17:10:46 jmcneill Exp $

DTSSUBDIR=	rockchip
DTSMAKEVARS=	CONFIG_ARCH_ROCKCHIP=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# For compatibility with factory firmware, copy rk3399-pinebook-pro.dtb to
# the base directory.
DTB_NOSUBDIR+=	rk3399-pinebook-pro.dtb

.include <bsd.dtb.mk>
