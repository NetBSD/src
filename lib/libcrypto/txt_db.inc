#	$NetBSD: txt_db.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/txt_db/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/txt_db

CFLAGS+=	-I${OPENSSLSRC}/crypto/txt_db

SRCS+=txt_db.c


