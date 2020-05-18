#	$NetBSD: Makefile,v 1.1 2020/05/18 21:19:36 jmcneill Exp $

DTSSUBDIR=	broadcom
DTSMAKEVARS=	CONFIG_ARCH_BCM2835=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
