#	$NetBSD: Makefile,v 1.6 2023/05/07 16:22:10 christos Exp $

.include "bsd.own.mk"

# OpenSSL libraries.
SUBDIR= libapps liblegacy libcrypto libcryptotest libdes

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.include <bsd.subdir.mk>
