#	$NetBSD: stack.inc,v 1.1.1.1.2.2 2000/07/17 06:00:01 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/stack/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/stack

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/stack

SRCS+=stack.c


