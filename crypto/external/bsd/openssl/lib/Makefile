#	$NetBSD: Makefile,v 1.4.10.1 2019/06/10 21:41:54 christos Exp $

.include "bsd.own.mk"

# OpenSSL libraries.
SUBDIR= libcrypto libcryptotest libdes

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.include <bsd.subdir.mk>
