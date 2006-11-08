#	$NetBSD: Makefile,v 1.17 2006/11/08 21:02:10 christos Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libevent libposix libpthread librt libssp libutil

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libcrypto libdes
.endif

.include <bsd.subdir.mk>
