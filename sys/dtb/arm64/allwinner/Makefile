#	$NetBSD: Makefile,v 1.1 2020/05/18 21:19:35 jmcneill Exp $

DTSSUBDIR=	allwinner
DTSMAKEVARS=	CONFIG_ARCH_SUNXI=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
