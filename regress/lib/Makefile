#	$NetBSD: Makefile,v 1.5.10.2 2000/10/07 03:19:18 itojun Exp $

.include <bsd.own.mk>

SUBDIR+= libc libposix

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
