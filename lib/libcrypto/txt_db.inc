#	$NetBSD: txt_db.inc,v 1.3 2000/07/16 07:16:24 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/txt_db/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/txt_db

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/txt_db

SRCS+=txt_db.c


