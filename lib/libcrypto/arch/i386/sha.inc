#	$NetBSD: sha.inc,v 1.1 2003/11/13 02:10:00 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/sha/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/sha
.PATH:	${.CURDIR}/arch/i386

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/sha -DSHA1_ASM

SRCS+=sha_dgst.c sha1dgst.c sha_one.c sha1_one.c sha1_586.S
