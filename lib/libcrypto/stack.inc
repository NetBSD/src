#	$NetBSD: stack.inc,v 1.3 2000/07/16 07:16:24 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/stack/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/stack

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/stack

SRCS+=stack.c


