#	$NetBSD: txt_db.inc,v 1.1.1.1.2.2 2000/07/17 06:00:01 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/txt_db/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/txt_db

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/txt_db

SRCS+=txt_db.c


