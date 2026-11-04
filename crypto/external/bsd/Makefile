#	$NetBSD: Makefile,v 1.14.16.2 2026/11/04 22:36:32 perseant Exp $

.include <bsd.own.mk>

.if ${HAVE_OPENSSL} < 35
SUBDIR+= ${EXTERNAL_OPENSSL_SUBDIR:T} 
.endif
SUBDIR+= .WAIT heimdal netpgp .WAIT libsaslc

.if (${MKSSH} != "no")
SUBDIR+= openssh
.endif

.include <bsd.subdir.mk>
