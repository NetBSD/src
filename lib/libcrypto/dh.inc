#	$NetBSD: dh.inc,v 1.2.2.2 2000/06/23 16:16:22 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/dh/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dh

CFLAGS+=	-I${OPENSSLSRC}/crypto/dh

SRCS+= dh_gen.c dh_key.c dh_lib.c dh_check.c dh_err.c


