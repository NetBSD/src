#	$NetBSD: Makefile,v 1.8 2023/05/26 22:27:50 riastradh Exp $

.include "bsd.own.mk"

# OpenSSL libraries.
SUBDIR= libapps libcommon libcrypto libcryptotest libdes

SUBDIR+= .WAIT	libssl liblegacy	# depends on libcrypto, libcommon
SUBDIR+= engines

.include <bsd.subdir.mk>
