#	$NetBSD: Makefile,v 1.15 2003/07/18 16:18:29 lukem Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libevent libposix libpthread librt libutil

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} != "no"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
