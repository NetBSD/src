#	$NetBSD: Makefile,v 1.2 2006/07/10 23:06:51 ross Exp $

.include <bsd.own.mk>

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+=	ppc psim
.endif

.include <bsd.subdir.mk>
