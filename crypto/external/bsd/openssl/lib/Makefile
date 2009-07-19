#	$NetBSD: Makefile,v 1.1 2009/07/19 23:30:42 christos Exp $

.include "bsd.own.mk"

.if (${MKCRYPTO} != "no")

# OpenSSL libraries.
SUBDIR= libcrypto libdes

.if (${MKCRYPTO_IDEA} != "no")
SUBDIR+= libcrypto_idea
.endif	# MKCRYPTO_IDEA != no

.if (${MKCRYPTO_MDC2} != "no")
SUBDIR+= libcrypto_mdc2
.endif	# MKCRYPTO_MDC2 != no

.if (${MKCRYPTO_RC5} != "no")
SUBDIR+= libcrypto_rc5
.endif	# MKCRYPTO_RC5 != no

SUBDIR+= .WAIT	libssl		# depends on libcrypto

.endif	# MKCRYPTO != no

.include <bsd.subdir.mk>
