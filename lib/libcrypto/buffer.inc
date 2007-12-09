#	$NetBSD: buffer.inc,v 1.6 2007/12/09 22:44:13 adrianp Exp $
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
