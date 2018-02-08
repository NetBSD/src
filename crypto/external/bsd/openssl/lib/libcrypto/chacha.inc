#	$NetBSD: chacha.inc,v 1.1 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/chacha/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/chacha


CHACHA_SRCS ?= \
poly_enc.c

SRCS += ${CHACHA_SRCS}

.for cryptosrc in ${CHACHA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/chacha ${CHACHA_CPPFLAGS}
.endfor
