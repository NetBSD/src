#	$NetBSD: cast.inc,v 1.2 2000/06/21 19:34:24 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/cast/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/cast

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/cast

SRCS+=c_skey.c c_ecb.c c_enc.c  c_cfb64.c c_ofb64.c


