#	$NetBSD: Makefile,v 1.4.8.1 2018/09/30 01:44:52 pgoyette Exp $

.include "bsd.own.mk"

# OpenSSL libraries.
SUBDIR= libcrypto libcryptotest libdes

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.include <bsd.subdir.mk>
