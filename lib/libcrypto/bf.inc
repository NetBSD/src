#	$NetBSD: bf.inc,v 1.9 2008/05/10 21:30:06 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/bf/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/bf

.if !defined(BF_SRCS)
BF_SRCS = bf_enc.c bf_ecb.c
.endif
BF_SRCS += bf_skey.c bf_cfb64.c bf_ofb64.c 
SRCS += ${BF_SRCS}

.for cryptosrc in ${BF_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/bf
.endfor
