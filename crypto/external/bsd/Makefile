#	$NetBSD: Makefile,v 1.16 2025/12/28 18:43:24 thorpej Exp $

.include <bsd.own.mk>

.if ${HAVE_OPENSSL} < 35
SUBDIR+= ${EXTERNAL_OPENSSL_SUBDIR:T} 
.endif
SUBDIR+= .WAIT heimdal netpgp .WAIT libsaslc

.if (${MKSSH} != "no")
SUBDIR+= openssh
.endif

.include <bsd.subdir.mk>
