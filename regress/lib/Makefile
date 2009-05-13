#	$NetBSD: Makefile,v 1.20.16.1 2009/05/13 19:18:44 jym Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libevent libm libposix libpthread librt libutil

.if (${MACHINE_CPU} != "alpha" && \
     ${MACHINE_CPU} != "mips" && \
     ${MACHINE_CPU} != "hppa")
#SUBDIR+= libssp
.endif

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libdes
.endif

.include <bsd.subdir.mk>
