#	$NetBSD: hmac.inc,v 1.4 2003/07/24 14:16:32 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/hmac/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/hmac

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/hmac

SRCS+=hmac.c
