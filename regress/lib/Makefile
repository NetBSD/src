#	$NetBSD: Makefile,v 1.21 2009/02/14 00:26:14 cube Exp $

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
