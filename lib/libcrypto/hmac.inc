#	$NetBSD: hmac.inc,v 1.5.10.1 2008/01/09 01:34:34 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/hmac/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/hmac


HMAC_SRCS = hmac.c
SRCS += ${HMAC_SRCS}

.for cryptosrc in ${HMAC_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/hmac
.endfor
