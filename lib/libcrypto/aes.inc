#	$NetBSD: aes.inc,v 1.2 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/aes/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/aes


AES_SRCS = aes_core.c aes_misc.c aes_ecb.c aes_cbc.c aes_cfb.c aes_ofb.c aes_ctr.c
SRCS += ${AES_SRCS}

.for cryptosrc in ${AES_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/aes
.endfor
