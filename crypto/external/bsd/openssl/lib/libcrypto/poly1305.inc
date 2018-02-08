#	$NetBSD: poly1305.inc,v 1.1 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/poly1305/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/poly1305


POLY1305_SRCS += \
poly1305.c

SRCS += ${POLY1305_SRCS}

.for cryptosrc in ${POLY1305_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/poly1305 ${POLY1305_CPPFLAGS}
.endfor
