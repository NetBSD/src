#	$NetBSD: bf.inc,v 1.7.10.1 2008/01/09 01:34:28 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/bf/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/bf


BF_SRCS = bf_skey.c bf_ecb.c bf_enc.c bf_cfb64.c bf_ofb64.c 
SRCS += ${BF_SRCS}

.for cryptosrc in ${BF_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/bf
.endfor
