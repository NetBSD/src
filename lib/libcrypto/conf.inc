#	$NetBSD: conf.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/conf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/conf

CFLAGS+=	-I${OPENSSLSRC}/crypto/conf

SRCS+=	conf.c conf_err.c


