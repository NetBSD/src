#	$NetBSD: idea.inc,v 1.2 2000/06/16 06:16:38 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /usr/src/openssl/crypto/idea/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/idea

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/idea

SRCS+=i_cbc.c i_cfb64.c i_ofb64.c i_ecb.c i_skey.c


