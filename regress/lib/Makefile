#	$NetBSD: Makefile,v 1.14 2003/06/12 23:18:09 provos Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libevent libposix libpthread librt libutil

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
