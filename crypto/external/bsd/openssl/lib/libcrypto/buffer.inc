#	$NetBSD: buffer.inc,v 1.3 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/buffer/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/buffer


BUFFER_SRCS = buffer.c buf_err.c
SRCS += ${BUFFER_SRCS}

.for cryptosrc in ${BUFFER_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/buffer
.endfor
