#	$NetBSD: modes.inc,v 1.2.2.1 2013/02/25 00:24:19 tls Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/modes/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/modes

MODES_SRCS += cbc128.c ctr128.c cts128.c cfb128.c ofb128.c gcm128.c \
    ccm128.c xts128.c


SRCS += ${MODES_SRCS}

.for cryptosrc in ${MODES_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/modes
.endfor
