#	$NetBSD: bf.inc,v 1.1.1.1.2.1 2000/06/22 06:55:03 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/bf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bf

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/bf

SRCS+=bf_skey.c bf_ecb.c   bf_enc.c  bf_cfb64.c bf_ofb64.c


