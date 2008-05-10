#	$NetBSD: aes.inc,v 1.6 2008/05/10 21:29:31 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/aes/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/aes

.if !defined(AES_SRCS)
AES_SRCS= aes_core.c aes_cbc.c
.endif

AES_SRCS += aes_misc.c aes_cfb.c aes_ofb.c \
    aes_ctr.c aes_wrap.c aes_ige.c aes_ecb.c
SRCS += ${AES_SRCS}

.for cryptosrc in ${AES_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/aes
.endfor

