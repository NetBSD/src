#	$NetBSD: stack.inc,v 1.2 2000/06/21 19:34:28 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/stack/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/stack

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/stack

SRCS+=stack.c


