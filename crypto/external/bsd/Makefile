#	$NetBSD: Makefile,v 1.11 2011/04/20 08:36:07 elric Exp $

.include <bsd.own.mk>

.if (${MKCRYPTO} != "no")
SUBDIR+= openssl .WAIT heimdal netpgp .WAIT openssh libsaslc
.endif

.include <bsd.subdir.mk>
