#	$NetBSD: aes.inc,v 1.4 2008/05/09 21:52:18 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/aes/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/aes


AES_SRCS = aes_core.c aes_misc.c aes_ecb.c aes_cbc.c aes_cfb.c aes_ofb.c \
    aes_ctr.c aes_wrap.c aes_ige.c
SRCS += ${AES_SRCS}

.for cryptosrc in ${AES_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/aes
.endfor
