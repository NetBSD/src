#	$NetBSD: rand.inc,v 1.2.2.2 2000/06/23 16:16:24 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/rand/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rand

CFLAGS+=	-I${OPENSSLSRC}/crypto/rand

SRCS+=md_rand.c randfile.c rand_lib.c


