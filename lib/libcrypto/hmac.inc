#	$NetBSD: hmac.inc,v 1.2.2.2 2000/06/23 16:16:23 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/hmac/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/hmac

CFLAGS+=	-I${OPENSSLSRC}/crypto/hmac

SRCS+=hmac.c


