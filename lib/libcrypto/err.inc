#	$NetBSD: err.inc,v 1.1.1.1.2.1 2000/06/22 06:55:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/err/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/err

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/err

SRCS+=err.c err_all.c err_prn.c


