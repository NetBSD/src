#	$NetBSD: Makefile,v 1.10 2011/04/15 18:05:42 elric Exp $

.include <bsd.own.mk>

.if (${MKCRYPTO} != "no")
SUBDIR+= openssl .WAIT heimdal netpgp openssh libsaslc
.endif

.include <bsd.subdir.mk>
