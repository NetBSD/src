#	$NetBSD: txt_db.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/txt_db/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/txt_db


TXT_DB_SRCS = txt_db.c
SRCS += ${TXT_DB_SRCS}

.for cryptosrc in ${TXT_DB_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/txt_db
.endfor
