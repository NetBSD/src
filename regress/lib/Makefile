#	$NetBSD: Makefile,v 1.9 2001/07/17 03:42:23 thorpej Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libposix libskey

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
