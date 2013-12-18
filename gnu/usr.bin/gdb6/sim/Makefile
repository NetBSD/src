#	$NetBSD: Makefile,v 1.3.30.1 2013/12/18 18:43:30 matt Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

# PSIM support for PowerPC.
.if ${GDB_MACHINE_ARCH} == powerpc || ${GDB_MACHINE_ARCH} == powerpcle || \
    ${GDB_MACHINE_ARCH} == powerpc64
SUBDIR+=	ppc .WAIT psim
.endif

.include <bsd.subdir.mk>
