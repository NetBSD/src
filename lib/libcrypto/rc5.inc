#	$NetBSD: rc5.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /usr/src/openssl/crypto/rc5/Makefile.ssl
#

.PATH:	${PATENTEDOPENSSLSRC}/crypto/rc5

CPPFLAGS+=	-I${PATENTEDOPENSSLSRC}/crypto/rc5

SRCS+=rc5_skey.c rc5_ecb.c   rc5_enc.c  rc5cfb64.c rc5ofb64.c


