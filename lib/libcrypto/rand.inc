#	$NetBSD: rand.inc,v 1.4 2001/04/12 07:48:05 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/rand/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rand

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rand

SRCS+=md_rand.c randfile.c rand_lib.c rand_err.c rand_egd.c rand_win.c
