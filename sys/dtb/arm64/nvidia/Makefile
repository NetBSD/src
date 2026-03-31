#	$NetBSD: Makefile,v 1.3 2026/03/31 20:24:22 yurix Exp $

# Important: Regenerate dtb set lists by running 'make update-sets' in
# sys/arch/dtb after changing this file.

DTSSUBDIR=	nvidia
DTSMAKEVARS=	CONFIG_ARCH_TEGRA_210_SOC=y CONFIG_ARCH_TEGRA_186_SOC=y
DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
