#	$NetBSD: bf.inc,v 1.6 2003/07/24 14:16:31 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/bf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bf

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/bf

SRCS+=bf_skey.c bf_ecb.c bf_enc.c bf_cfb64.c bf_ofb64.c 
