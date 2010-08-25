#	$NetBSD: Makefile,v 1.26 2010/08/25 16:47:43 jmmv Exp $

.include <bsd.own.mk>

SUBDIR+= libc libm

.if (${MACHINE_CPU} != "alpha" && \
     ${MACHINE_CPU} != "mips" && \
     ${MACHINE_CPU} != "hppa")
#SUBDIR+= libssp
.endif

.include <bsd.subdir.mk>
