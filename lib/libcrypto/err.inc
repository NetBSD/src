#	$NetBSD: err.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/err/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/err

CFLAGS+=	-I${OPENSSLSRC}/crypto/err

SRCS+=err.c err_all.c err_prn.c


