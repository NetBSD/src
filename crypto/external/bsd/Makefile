#	$NetBSD: Makefile,v 1.8 2010/03/20 02:07:33 uwe Exp $

.include <bsd.own.mk>

.if (${MKCRYPTO} != "no")
SUBDIR+= openssl .WAIT netpgp openssh
.endif

.include <bsd.subdir.mk>
