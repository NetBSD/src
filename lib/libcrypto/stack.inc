#	$NetBSD: stack.inc,v 1.1.1.1.2.1 2000/06/22 06:55:05 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/stack/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/stack

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/stack

SRCS+=stack.c


