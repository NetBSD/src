#	$NetBSD: Makefile,v 1.5.10.1 2000/10/07 01:16:08 itojun Exp $

.include <bsd.own.mk>

SUBDIR+= libc libposix libskey

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
