#	$NetBSD: Makefile,v 1.18 2006/11/11 23:37:18 he Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libevent libposix libpthread librt libutil

.if (${MACHINE_CPU} != "alpha" && \
     ${MACHINE_CPU} != "mips" && \
     ${MACHINE_CPU} != "hppa")
SUBDIR+= libssp
.endif

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libcrypto libdes
.endif

.include <bsd.subdir.mk>
