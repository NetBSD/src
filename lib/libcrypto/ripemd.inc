#	$NetBSD: ripemd.inc,v 1.4 2003/07/24 14:16:33 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ripemd/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ripemd

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ripemd

SRCS+=rmd_dgst.c rmd_one.c
