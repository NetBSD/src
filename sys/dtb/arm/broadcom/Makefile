#	$NetBSD: Makefile,v 1.3 2026/03/31 20:24:20 yurix Exp $

# Important: Regenerate dtb set lists by running 'make update-sets' in
# sys/arch/dtb after changing this file.

DTSSUBDIR=	broadcom
.if !empty(MACHINE_ARCH:Mearmv6*)
DTSMAKEVARS=	CONFIG_ARCH_BCM2835=y
.elif !empty(MACHINE_ARCH:Mearmv7*)
DTSMAKEVARS=	CONFIG_ARCH_BCM2835=y
.endif

DTSFILESCMD=	${MAKE} -C ${ARCHDTSDIR}/${DTSSUBDIR} ${DTSMAKEVARS} -v dtb-y
DTS=		${DTSFILESCMD:sh}

.include <bsd.dtb.mk>
