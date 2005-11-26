#	$NetBSD: sha.inc,v 1.3 2005/11/26 00:35:58 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/sha/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/sha
.PATH:	${.CURDIR}/arch/i386

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/sha -DSHA1_ASM

SRCS+=sha_dgst.c sha1dgst.c sha_one.c sha1_one.c sha1_586.S sha256.c sha512.c
