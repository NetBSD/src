#	$NetBSD: hmac.inc,v 1.1.1.1.2.1 2000/06/22 06:55:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/hmac/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/hmac

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/hmac

SRCS+=hmac.c


