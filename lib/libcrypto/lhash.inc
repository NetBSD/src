#	$NetBSD: lhash.inc,v 1.2 2000/06/21 19:34:25 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/lhash/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/lhash

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/lhash

SRCS+=lhash.c lh_stats.c


