#	$NetBSD: lhash.inc,v 1.3 2000/07/16 07:16:22 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/lhash/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/lhash

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/lhash

SRCS+=lhash.c lh_stats.c


