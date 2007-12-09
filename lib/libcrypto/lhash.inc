#	$NetBSD: lhash.inc,v 1.6 2007/12/09 22:44:17 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/lhash/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/lhash


LHASH_SRCS = lhash.c lh_stats.c
SRCS += ${LHASH_SRCS}

.for cryptosrc in ${LHASH_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/lhash
.endfor
