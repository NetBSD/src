#	$NetBSD: rc2.inc,v 1.1.1.1.2.2 2000/07/17 06:00:00 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/rc2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc2

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rc2

SRCS+=rc2_ecb.c rc2_skey.c rc2_cbc.c rc2cfb64.c rc2ofb64.c


