#	$NetBSD: Makefile,v 1.8 2000/10/04 00:49:44 itojun Exp $

.include <bsd.own.mk>

SUBDIR+= libc libposix libskey

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
