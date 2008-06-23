#	$NetBSD: bf.inc,v 1.8.8.1 2008/06/23 04:29:36 wrstuden Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/bf/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/bf

.if !defined(BF_SRCS)
BF_SRCS = bf_enc.c 
.endif
BF_SRCS += bf_skey.c bf_ecb.c bf_cfb64.c bf_ofb64.c 
SRCS += ${BF_SRCS}

.for cryptosrc in ${BF_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/bf
.endfor
