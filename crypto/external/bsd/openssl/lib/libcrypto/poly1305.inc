#	$NetBSD: poly1305.inc,v 1.1.2.1 2018/09/30 01:44:52 pgoyette Exp $
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
