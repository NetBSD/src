#	$NetBSD: Makefile,v 1.1 2024/08/12 10:55:51 skrll Exp $

DTSSUBDIR=	allwinner
DTSMAKEVARS=	CONFIG_ARCH_SUNXI=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
