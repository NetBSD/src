#	$NetBSD: Makefile,v 1.16 2003/08/04 04:20:25 itojun Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libevent libposix libpthread librt libutil

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libcrypto libdes
.endif

.include <bsd.subdir.mk>
