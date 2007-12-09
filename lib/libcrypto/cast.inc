#	$NetBSD: cast.inc,v 1.6 2007/12/09 22:44:14 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/cast/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/cast


CAST_SRCS = c_skey.c c_ecb.c c_enc.c c_cfb64.c c_ofb64.c 
SRCS += ${CAST_SRCS}

.for cryptosrc in ${CAST_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/cast
.endfor
