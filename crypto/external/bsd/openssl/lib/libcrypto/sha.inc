#	$NetBSD: sha.inc,v 1.5 2018/04/06 23:04:46 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/sha/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/sha


SHA_SRCS += sha1dgst.c sha1_one.c

# Replaced OpenSSL version to avoid overlap with libc
SHA_SRCS+= libc-sha512.c libc-sha256.c
#SHA_SRCS+= sha256.c sha512.c

SRCS += ${SHA_SRCS}

.for cryptosrc in ${SHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sha ${SHACPPFLAGS}
.endfor
