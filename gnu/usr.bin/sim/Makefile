#	$NetBSD: Makefile,v 1.3 2002/02/10 17:38:24 tsutsui Exp $

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

# PSIM support for PowerPC.
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
SUBDIR+=	ppc
.endif

.endif

.include <bsd.subdir.mk>
