#	$NetBSD: rc2.inc,v 1.3 2000/07/16 07:16:23 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/rc2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc2

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rc2

SRCS+=rc2_ecb.c rc2_skey.c rc2_cbc.c rc2cfb64.c rc2ofb64.c


