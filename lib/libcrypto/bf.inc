#	$NetBSD: bf.inc,v 1.3 2000/07/16 07:16:21 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/bf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bf

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/bf

SRCS+=bf_skey.c bf_ecb.c   bf_enc.c  bf_cfb64.c bf_ofb64.c


