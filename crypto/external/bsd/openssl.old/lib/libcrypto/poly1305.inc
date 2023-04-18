#	$NetBSD: poly1305.inc,v 1.1.1.1 2023/04/18 14:19:04 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/poly1305/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/poly1305


POLY1305_SRCS += \
poly1305.c \
poly1305_ameth.c \
poly1305_pmeth.c

SRCS += ${POLY1305_SRCS}

.for cryptosrc in ${POLY1305_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/poly1305 ${POLY1305_CPPFLAGS}
.endfor
