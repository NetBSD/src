#	$NetBSD: hmac.inc,v 1.1.1.1.2.2 2000/07/17 05:59:59 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/hmac/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/hmac

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/hmac

SRCS+=hmac.c


