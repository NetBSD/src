#	$NetBSD: rand.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/rand/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rand

CFLAGS+=	-I${OPENSSLSRC}/crypto/rand

SRCS+=md_rand.c randfile.c rand_lib.c


