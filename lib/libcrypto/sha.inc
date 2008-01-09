#	$NetBSD: sha.inc,v 1.8.10.1 2008/01/09 01:34:40 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/sha/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/sha


SHA_SRCS = sha_dgst.c sha1dgst.c sha_one.c sha1_one.c sha256.c sha512.c
SRCS += ${SHA_SRCS}

.for cryptosrc in ${SHA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/sha
.endfor
