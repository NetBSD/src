#	$NetBSD: bf.inc,v 1.2.2.2 2000/06/23 16:16:20 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/bf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bf

CFLAGS+=	-I${OPENSSLSRC}/crypto/bf

SRCS+=bf_skey.c bf_ecb.c   bf_enc.c  bf_cfb64.c bf_ofb64.c


