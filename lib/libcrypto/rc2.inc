#	$NetBSD: rc2.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/rc2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rc2

CFLAGS+=	-I${OPENSSLSRC}/crypto/rc2

SRCS+=rc2_ecb.c rc2_skey.c rc2_cbc.c rc2cfb64.c rc2ofb64.c


