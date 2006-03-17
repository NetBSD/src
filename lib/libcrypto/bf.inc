#	$NetBSD: bf.inc,v 1.7 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/bf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/bf


BF_SRCS = bf_skey.c bf_ecb.c bf_enc.c bf_cfb64.c bf_ofb64.c 
SRCS += ${BF_SRCS}

.for cryptosrc in ${BF_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/bf
.endfor
