#	$NetBSD: chacha.inc,v 1.2 2018/02/10 16:16:28 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/chacha/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/chacha


CHACHA_SRCS ?= \
chacha_enc.c

SRCS += ${CHACHA_SRCS}

.for cryptosrc in ${CHACHA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/chacha ${CHACHA_CPPFLAGS}
.endfor
