#	$NetBSD: krb5.inc,v 1.1 2003/07/24 14:16:32 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/krb5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/krb5

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/krb5

SRCS+=	krb5_asn.c
