#	$NetBSD: lhash.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/lhash/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/lhash


LHASH_SRCS = lhash.c lh_stats.c
SRCS += ${LHASH_SRCS}

.for cryptosrc in ${LHASH_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/lhash
.endfor
