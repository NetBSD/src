#	$NetBSD: stack.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/stack/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/stack

CFLAGS+=	-I${OPENSSLSRC}/crypto/stack

SRCS+=stack.c


