#	$NetBSD: Makefile,v 1.9 2011/02/12 19:07:35 christos Exp $

.include <bsd.own.mk>

.if (${MKCRYPTO} != "no")
SUBDIR+= openssl .WAIT netpgp openssh libsaslc
.endif

.include <bsd.subdir.mk>
