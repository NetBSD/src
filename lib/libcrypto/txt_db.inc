#	$NetBSD: txt_db.inc,v 1.2.2.2 2000/06/23 16:16:26 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/txt_db/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/txt_db

CFLAGS+=	-I${OPENSSLSRC}/crypto/txt_db

SRCS+=txt_db.c


