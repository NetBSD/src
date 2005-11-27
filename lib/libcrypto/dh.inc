#	$NetBSD: dh.inc,v 1.5 2005/11/27 02:00:46 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/dh/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dh

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/dh

SRCS+= dh_asn1.c dh_gen.c dh_key.c dh_lib.c dh_check.c dh_err.c dh_depr.c
