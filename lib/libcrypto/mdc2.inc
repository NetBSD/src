#	$NetBSD: mdc2.inc,v 1.2 2000/06/21 19:34:25 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/mdc2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/mdc2

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/mdc2

SRCS+=mdc2dgst.c mdc2_one.c


