#	$NetBSD: blake2.inc,v 1.1 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/blake2/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/blake2

BLAKE2_SRCS = \
blake2b.c \
blake2s.c \
m_blake2b.c \
m_blake2s.c

SRCS += ${BLAKE2_SRCS}

.for cryptosrc in ${BLAKE2_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/blake2
.endfor
