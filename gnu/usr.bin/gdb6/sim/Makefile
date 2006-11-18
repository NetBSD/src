#	$NetBSD: Makefile,v 1.3 2006/11/18 20:19:24 he Exp $

.include <bsd.own.mk>

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+=	ppc .WAIT psim
.endif

.include <bsd.subdir.mk>
