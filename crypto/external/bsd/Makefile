#	$NetBSD: Makefile,v 1.15 2025/07/13 20:15:15 christos Exp $

.include <bsd.own.mk>

.if ${HAVE_OPENSSL} < 35
SUBDIR+= ${EXTERNAL_OPENSSL_SUBDIR:T} 
.endif
SUBDIR+= .WAIT heimdal netpgp .WAIT openssh libsaslc

.include <bsd.subdir.mk>
