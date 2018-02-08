#	$NetBSD: err.inc,v 1.2 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/err/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/err


ERR_SRCS = \
err.c \
err_all.c \
err_prn.c

SRCS += ${ERR_SRCS}

.for cryptosrc in ${ERR_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/err
.endfor
