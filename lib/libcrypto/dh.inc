#	$NetBSD: dh.inc,v 1.1.1.1.2.2 2000/07/17 05:59:58 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/dh/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dh

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/dh

SRCS+= dh_gen.c dh_key.c dh_lib.c dh_check.c dh_err.c


