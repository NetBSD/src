#	$NetBSD: md5.inc,v 1.3 2000/07/16 07:16:22 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/md5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md5

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/md5

SRCS+=md5_dgst.c md5_one.c $(MD5_ASM_OBJ)


