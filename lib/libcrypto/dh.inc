#	$NetBSD: dh.inc,v 1.4 2003/07/24 14:16:32 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/dh/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dh

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/dh

SRCS+= dh_asn1.c dh_gen.c dh_key.c dh_lib.c dh_check.c dh_err.c
