#	$NetBSD: conf.inc,v 1.2 2000/06/21 19:34:24 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/conf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/conf

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/conf

SRCS+=	conf.c conf_err.c


