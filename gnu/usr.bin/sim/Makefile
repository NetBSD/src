#	$NetBSD: Makefile,v 1.2 2002/01/22 17:02:08 mrg Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
SUBDIR+=	ppc
.endif

.include <bsd.subdir.mk>

.endif
