#	$NetBSD: Makefile,v 1.11 2002/09/20 21:13:28 christos Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libposix libutil

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
