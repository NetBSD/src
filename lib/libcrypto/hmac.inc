#	$NetBSD: hmac.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/hmac/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/hmac

CFLAGS+=	-I${OPENSSLSRC}/crypto/hmac

SRCS+=hmac.c


