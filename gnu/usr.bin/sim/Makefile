#	$NetBSD: Makefile,v 1.1 2002/01/22 16:55:20 mrg Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
SUBDIR+=	ppc .WAIT ppc/psim
.endif

.include <bsd.subdir.mk>

.endif
