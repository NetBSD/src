#	$NetBSD: Makefile,v 1.3.10.1 2026/11/04 23:22:41 perseant Exp $

# Important: Regenerate dtb set lists by running 'make update-sets' in
# sys/arch/dtb after changing this file.

DTSSUBDIR=	rockchip
DTSMAKEVARS=	CONFIG_ARCH_ROCKCHIP=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

# For compatibility with factory firmware, copy rk3399-pinebook-pro.dtb to
# the base directory.
DTB_NOSUBDIR+=	rk3399-pinebook-pro.dtb

.include <bsd.dtb.mk>
