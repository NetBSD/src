#	$NetBSD: aes.inc,v 1.1 2003/07/24 14:16:31 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/aes/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/aes

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/aes

SRCS+=aes_core.c aes_misc.c aes_ecb.c aes_cbc.c aes_cfb.c aes_ofb.c aes_ctr.c
