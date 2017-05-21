#	$NetBSD: Makefile,v 1.3 2017/05/21 14:20:44 riastradh Exp $

.include "bsd.own.mk"

.if (${MKCRYPTO} != "no")

# OpenSSL libraries.
SUBDIR= libcrypto libdes

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.endif	# MKCRYPTO != no

.include <bsd.subdir.mk>
