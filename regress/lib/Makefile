#	$NetBSD: Makefile,v 1.27 2010/12/20 23:48:25 pgoyette Exp $

.include <bsd.own.mk>

SUBDIR+= libc

.if (${MACHINE_CPU} != "alpha" && \
     ${MACHINE_CPU} != "mips" && \
     ${MACHINE_CPU} != "hppa")
#SUBDIR+= libssp
.endif

.include <bsd.subdir.mk>
