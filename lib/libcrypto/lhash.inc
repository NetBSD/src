#	$NetBSD: lhash.inc,v 1.2.2.2 2000/06/23 16:16:23 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/lhash/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/lhash

CFLAGS+=	-I${OPENSSLSRC}/crypto/lhash

SRCS+=lhash.c lh_stats.c


