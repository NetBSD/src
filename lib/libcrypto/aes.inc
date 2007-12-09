#	$NetBSD: aes.inc,v 1.3 2007/12/09 22:44:12 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/aes/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/aes


AES_SRCS = aes_core.c aes_misc.c aes_ecb.c aes_cbc.c aes_cfb.c aes_ofb.c aes_ctr.c
SRCS += ${AES_SRCS}

.for cryptosrc in ${AES_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/aes
.endfor
