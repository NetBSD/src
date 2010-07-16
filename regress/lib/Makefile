#	$NetBSD: Makefile,v 1.24 2010/07/16 15:46:01 jmmv Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libm

.if (${MACHINE_CPU} != "alpha" && \
     ${MACHINE_CPU} != "mips" && \
     ${MACHINE_CPU} != "hppa")
#SUBDIR+= libssp
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libdes
.endif

.include <bsd.subdir.mk>
