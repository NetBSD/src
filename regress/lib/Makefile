#	$NetBSD: Makefile,v 1.5.10.3 2001/12/09 17:25:46 he Exp $

.include <bsd.own.mk>

SUBDIR+= csu libc libposix

.if ${MKCRYPTO} == "yes"
SUBDIR+= libcrypto 
.endif

.include <bsd.subdir.mk>
