#	$NetBSD: Makefile,v 1.1 2020/05/18 21:19:36 jmcneill Exp $

DTSSUBDIR=	freescale
DTSMAKEVARS=	CONFIG_ARCH_MXC=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
