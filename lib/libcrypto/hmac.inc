#	$NetBSD: hmac.inc,v 1.2 2000/06/21 19:34:25 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/hmac/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/hmac

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/hmac

SRCS+=hmac.c


