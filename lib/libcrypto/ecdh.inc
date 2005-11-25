#	$NetBSD: ecdh.inc,v 1.1 2005/11/25 19:15:08 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ec/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ecdh

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ecdh

SRCS+=	ech_err.c ech_key.c ech_lib.c ech_ossl.c 
