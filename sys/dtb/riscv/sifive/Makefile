#	$NetBSD: Makefile,v 1.1.10.2 2026/11/04 23:22:42 perseant Exp $

# Important: Regenerate dtb set lists by running 'make update-sets' in
# sys/arch/dtb after changing this file.

DTSSUBDIR=	sifive
DTSMAKEVARS=	CONFIG_ARCH_SIFIVE=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
