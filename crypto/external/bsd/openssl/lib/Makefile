#	$NetBSD: Makefile,v 1.1.8.1.4.1 2015/04/02 17:58:48 snj Exp $

.include "bsd.own.mk"

.if (${MKCRYPTO} != "no")

# OpenSSL libraries.
SUBDIR= libcrypto libdes

.if (${MKCRYPTO_RC5} != "no")
SUBDIR+= libcrypto_rc5
.endif	# MKCRYPTO_RC5 != no

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.endif	# MKCRYPTO != no

.include <bsd.subdir.mk>
