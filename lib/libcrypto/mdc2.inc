#	$NetBSD: mdc2.inc,v 1.4 2001/04/12 07:48:05 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/mdc2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/mdc2

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/mdc2

SRCS+=	mdc2dgst.c mdc2_one.c
