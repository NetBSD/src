#	$NetBSD: err.inc,v 1.2 2000/06/21 19:34:25 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/err/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/err

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/err

SRCS+=err.c err_all.c err_prn.c


