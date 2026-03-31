#	$NetBSD: Makefile,v 1.4 2026/03/31 20:24:23 yurix Exp $

# Important: Regenerate dtb set lists by running 'make update-sets' in
# sys/arch/dtb after changing this file.

DTSSUBDIR=	sophgo
DTSMAKEVARS=	CONFIG_ARCH_SOPHGO=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
