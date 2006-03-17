#	$NetBSD: cast.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/cast/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/cast


CAST_SRCS = c_skey.c c_ecb.c c_enc.c c_cfb64.c c_ofb64.c 
SRCS += ${CAST_SRCS}

.for cryptosrc in ${CAST_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/cast
.endfor
