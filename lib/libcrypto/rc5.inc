#	$NetBSD: rc5.inc,v 1.2.2.1 2000/07/17 06:00:00 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/rc5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc5

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rc5

SRCS+=rc5_skey.c rc5_ecb.c   rc5_enc.c  rc5cfb64.c rc5ofb64.c


