#	$NetBSD: Makefile,v 1.8.2.1 2011/02/17 11:57:11 bouyer Exp $

.include <bsd.own.mk>

.if (${MKCRYPTO} != "no")
SUBDIR+= openssl .WAIT netpgp openssh libsaslc
.endif

.include <bsd.subdir.mk>
