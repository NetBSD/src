#	$NetBSD: rand.inc,v 1.3 2000/07/16 07:16:23 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/rand/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rand

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rand

SRCS+=md_rand.c randfile.c rand_lib.c rand_err.c rand_egd.c


