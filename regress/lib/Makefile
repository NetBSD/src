#	$NetBSD: Makefile,v 1.10 2002/03/22 18:10:22 thorpej Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libposix

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
