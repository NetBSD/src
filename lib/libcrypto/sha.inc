#	$NetBSD: sha.inc,v 1.3 2000/07/16 07:16:24 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/sha/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/sha

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/sha

SRCS+=sha_dgst.c sha1dgst.c sha_one.c sha1_one.c $(SHA1_ASM_OBJ)


