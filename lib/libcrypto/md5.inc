#	$NetBSD: md5.inc,v 1.1.1.1 2000/06/16 04:21:32 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/md5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md5

CFLAGS+=	-I${OPENSSLSRC}/crypto/md5

SRCS+=md5_dgst.c md5_one.c $(MD5_ASM_OBJ)


