#	$NetBSD: Makefile,v 1.1 2006/05/15 16:31:37 nathanw Exp $

.include <bsd.own.mk>

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
SUBDIR+=	ppc psim
.endif

.include <bsd.subdir.mk>
