#	$NetBSD: bf.inc,v 1.1 2000/07/31 18:39:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/bf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bf
.PATH:	${.CURDIR}/arch/i386

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/bf
CPPFLAGS+=	-I${.CURDIR}/arch/i386

SRCS+=bf_skey.c bf_ecb.c bf_enc_x86.S bf_cbc.c bf_cfb64.c bf_ofb64.c
