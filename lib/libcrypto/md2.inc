#	$NetBSD: md2.inc,v 1.1.1.1.2.1 2000/06/22 06:55:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/md2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md2

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/md2

SRCS+=md2_dgst.c md2_one.c


