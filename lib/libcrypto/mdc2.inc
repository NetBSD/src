#	$NetBSD: mdc2.inc,v 1.2.2.2 2000/06/23 16:16:23 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/mdc2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/mdc2

CFLAGS+=	-I${OPENSSLSRC}/crypto/mdc2

SRCS+=mdc2dgst.c mdc2_one.c


