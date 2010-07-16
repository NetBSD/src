#	$NetBSD: Makefile,v 1.23 2010/07/16 13:59:07 jmmv Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libm libpthread

.if (${MACHINE_CPU} != "alpha" && \
     ${MACHINE_CPU} != "mips" && \
     ${MACHINE_CPU} != "hppa")
#SUBDIR+= libssp
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libdes
.endif

.include <bsd.subdir.mk>
