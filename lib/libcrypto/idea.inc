#	$NetBSD: idea.inc,v 1.2.2.1 2000/07/17 06:00:00 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/idea/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/idea

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/idea

SRCS+=i_cbc.c i_cfb64.c i_ofb64.c i_ecb.c i_skey.c


