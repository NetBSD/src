#	$NetBSD: Makefile,v 1.1.26.1 2026/11/04 23:22:40 perseant Exp $

# Important: Regenerate dtb set lists by running 'make update-sets' in
# sys/arch/dtb after changing this file.

DTSSUBDIR=	allwinner
DTSMAKEVARS=	CONFIG_ARCH_SUNXI=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
