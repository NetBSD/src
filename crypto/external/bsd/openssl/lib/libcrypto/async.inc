#	$NetBSD: async.inc,v 1.1 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/asn1/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/async ${OPENSSLSRC}/crypto/async/arch


ASYNC_SRCS = \
async.c \
async_err.c \
async_posix.c \
async_wait.c

SRCS += ${ASYNC_SRCS}

.for cryptosrc in ${ASYNC_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/async
.endfor
