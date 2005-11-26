#	$NetBSD: sha.inc,v 1.7 2005/11/26 00:35:58 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/sha/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/sha

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/sha

SRCS+=sha_dgst.c sha1dgst.c sha_one.c sha1_one.c sha256.c sha512.c
