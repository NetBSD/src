#	$NetBSD: Makefile,v 1.1.1.1 2018/02/03 22:43:37 christos Exp $

.include "bsd.own.mk"

# OpenSSL libraries.
SUBDIR= libcrypto libdes

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.include <bsd.subdir.mk>
