#	$NetBSD: lhash.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/lhash/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/lhash

CFLAGS+=	-I${OPENSSLSRC}/crypto/lhash

SRCS+=lhash.c lh_stats.c


