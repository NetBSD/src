#	$NetBSD: sha.inc,v 1.1.1.1.2.1 2000/06/22 06:55:05 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/sha/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/sha

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/sha

SRCS+=sha_dgst.c sha1dgst.c sha_one.c sha1_one.c $(SHA1_ASM_OBJ)


