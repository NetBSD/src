#	$NetBSD: Makefile,v 1.13 2003/01/24 01:56:47 thorpej Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libposix libpthread librt libutil

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
