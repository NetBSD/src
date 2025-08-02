#	$NetBSD: Makefile,v 1.14.16.1 2025/08/02 05:18:39 perseant Exp $

.include <bsd.own.mk>

.if ${HAVE_OPENSSL} < 35
SUBDIR+= ${EXTERNAL_OPENSSL_SUBDIR:T} 
.endif
SUBDIR+= .WAIT heimdal netpgp .WAIT openssh libsaslc

.include <bsd.subdir.mk>
