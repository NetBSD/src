#	$NetBSD: dh.inc,v 1.1.1.1.2.1 2000/06/22 06:55:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/dh/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dh

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/dh

SRCS+= dh_gen.c dh_key.c dh_lib.c dh_check.c dh_err.c


