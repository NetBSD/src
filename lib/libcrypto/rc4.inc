#	$NetBSD: rc4.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/rc4/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc4

CFLAGS+=	-I${OPENSSLSRC}/crypto/rc4

SRCS+=rc4_skey.c rc4_enc.c 


