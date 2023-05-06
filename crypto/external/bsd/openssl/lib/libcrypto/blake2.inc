#	$NetBSD: blake2.inc,v 1.2 2023/05/06 17:07:22 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/blake2/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/blake2

BLAKE2_SRCS = \
blake2_prov.c \
blake2b_prov.c \
blake2s_prov.c \
blake2b_mac.c \
blake2s_mac.c

SRCS += ${BLAKE2_SRCS}

.for cryptosrc in ${BLAKE2_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/blake2
.endfor
