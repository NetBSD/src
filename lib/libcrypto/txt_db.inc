#	$NetBSD: txt_db.inc,v 1.2 2000/06/21 19:34:28 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/txt_db/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/txt_db

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/txt_db

SRCS+=txt_db.c


