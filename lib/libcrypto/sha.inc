#	$NetBSD: sha.inc,v 1.2.2.2 2000/06/23 16:16:25 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/sha/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/sha

CFLAGS+=	-I${OPENSSLSRC}/crypto/sha

SRCS+=sha_dgst.c sha1dgst.c sha_one.c sha1_one.c $(SHA1_ASM_OBJ)


