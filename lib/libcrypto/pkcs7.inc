#	$NetBSD: pkcs7.inc,v 1.2.2.2 2000/06/23 16:16:24 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/pkcs7/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/pkcs7

CFLAGS+=	-I${OPENSSLSRC}/crypto/pkcs7

SRCS+= pk7_lib.c pkcs7err.c pk7_doit.c


