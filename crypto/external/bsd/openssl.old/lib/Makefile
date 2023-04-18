#	$NetBSD: Makefile,v 1.1.1.2 2023/04/18 14:19:03 christos Exp $

.include "bsd.own.mk"

# OpenSSL libraries.
SUBDIR= libcrypto libcryptotest libdes

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.include <bsd.subdir.mk>
