#	$NetBSD: rc4.inc,v 1.3 2000/07/16 07:16:23 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/rc4/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc4

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rc4

SRCS+=rc4_skey.c rc4_enc.c 


