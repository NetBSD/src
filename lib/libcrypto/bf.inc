#	$NetBSD: bf.inc,v 1.2 2000/06/21 19:34:23 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/bf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bf

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/bf

SRCS+=bf_skey.c bf_ecb.c   bf_enc.c  bf_cfb64.c bf_ofb64.c


