#	$NetBSD: conf.inc,v 1.2.2.2 2000/06/23 16:16:22 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/conf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/conf

CFLAGS+=	-I${OPENSSLSRC}/crypto/conf

SRCS+=	conf.c conf_err.c


