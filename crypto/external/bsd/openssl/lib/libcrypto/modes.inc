#	$NetBSD: modes.inc,v 1.1.8.1 2012/08/14 07:57:03 riz Exp $
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
CPPFLAGS.e_aes.c = -I${OPENSSLSRC}/crypto/modes
