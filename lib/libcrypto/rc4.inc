#	$NetBSD: rc4.inc,v 1.2 2000/06/21 19:34:26 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/rc4/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc4

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rc4

SRCS+=rc4_skey.c rc4_enc.c 


