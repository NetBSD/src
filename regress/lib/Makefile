#	$NetBSD: Makefile,v 1.12 2003/01/20 20:12:17 christos Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libposix libpthread libutil

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
