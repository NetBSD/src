#	$NetBSD: seed.inc,v 1.1.2.3 2015/04/02 17:47:51 snj Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/seed/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/seed

SEED_SRCS = seed.c seed_ecb.c seed_cbc.c seed_cfb.c seed_ofb.c

SRCS += ${SEED_SRCS}

.for cryptosrc in ${SEED_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/seed
.endfor
