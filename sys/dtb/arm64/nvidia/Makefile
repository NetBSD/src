#	$NetBSD: Makefile,v 1.2.10.1 2026/11/04 23:22:41 perseant Exp $

# Important: Regenerate dtb set lists by running 'make update-sets' in
# sys/arch/dtb after changing this file.

DTSSUBDIR=	nvidia
DTSMAKEVARS=	CONFIG_ARCH_TEGRA_210_SOC=y CONFIG_ARCH_TEGRA_186_SOC=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
