#	$NetBSD: modes.inc,v 1.6 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/modes/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/modes

MODES_SRCS += \
cbc128.c \
ccm128.c \
cfb128.c \
ctr128.c \
cts128.c \
gcm128.c \
ocb128.c \
ofb128.c \
wrap128.c \
xts128.c


SRCS += ${MODES_SRCS}

.for cryptosrc in ${MODES_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/modes ${MODESCPPFLAGS}
.endfor
