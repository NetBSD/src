#	$NetBSD: idea.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /usr/src/openssl/crypto/idea/Makefile.ssl
#

.PATH:	${PATENTEDOPENSSLSRC}/crypto/idea

CPPFLAGS+=	-I${PATENTEDOPENSSLSRC}/crypto/idea

SRCS+=i_cbc.c i_cfb64.c i_ofb64.c i_ecb.c i_skey.c


