#	$NetBSD: stack.inc,v 1.2.2.2 2000/06/23 16:16:26 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/stack/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/stack

CFLAGS+=	-I${OPENSSLSRC}/crypto/stack

SRCS+=stack.c


