#	$NetBSD: stack.inc,v 1.4 2003/07/24 14:16:33 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/stack/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/stack

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/stack

SRCS+=stack.c
