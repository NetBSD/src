#	$NetBSD: rc5.inc,v 1.2.4.2 2000/06/23 16:16:24 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /usr/src/openssl/crypto/rc5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc5

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rc5

SRCS+=rc5_skey.c rc5_ecb.c   rc5_enc.c  rc5cfb64.c rc5ofb64.c


