#	$NetBSD: cast.inc,v 1.1.1.1.2.2 2000/07/17 05:59:58 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/cast/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/cast

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/cast

SRCS+=c_skey.c c_ecb.c c_enc.c  c_cfb64.c c_ofb64.c


