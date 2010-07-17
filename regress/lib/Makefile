#	$NetBSD: Makefile,v 1.25 2010/07/17 19:29:35 jmmv Exp $

.include <bsd.own.mk>

SUBDIR+= libc libm

.if (${MACHINE_CPU} != "alpha" && \
     ${MACHINE_CPU} != "mips" && \
     ${MACHINE_CPU} != "hppa")
#SUBDIR+= libssp
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libdes
.endif

.include <bsd.subdir.mk>
