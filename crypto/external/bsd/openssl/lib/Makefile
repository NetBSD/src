#	$NetBSD: Makefile,v 1.5 2018/09/23 15:08:41 christos Exp $

.include "bsd.own.mk"

# OpenSSL libraries.
SUBDIR= libcrypto libcryptotest libdes

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.include <bsd.subdir.mk>
