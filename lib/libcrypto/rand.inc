#	$NetBSD: rand.inc,v 1.1.1.1.2.1 2000/06/22 06:55:05 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/rand/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rand

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rand

SRCS+=md_rand.c randfile.c rand_lib.c


