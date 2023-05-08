#	$NetBSD: Makefile,v 1.7 2023/05/08 14:30:33 christos Exp $

.include "bsd.own.mk"

# OpenSSL libraries.
SUBDIR= libapps libcommon libcrypto libcryptotest libdes

SUBDIR+= .WAIT	libssl liblegacy	# depends on libcrypto, libcommon

.include <bsd.subdir.mk>
