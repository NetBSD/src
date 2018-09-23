#	$NetBSD: sha.inc,v 1.6 2018/09/23 13:33:04 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/sha/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/sha


SHA_SRCS += sha1dgst.c sha1_one.c

# Replaced OpenSSL version to avoid overlap with libc
SHA_SRCS+= libc-sha512.c libc-sha256.c libc-sha2xx.c
#SHA_SRCS+= sha256.c sha512.c
.if !defined(KECCAKNI)
SHA_SRCS+= keccak1600.c
.endif

SRCS += ${SHA_SRCS}

.for cryptosrc in ${SHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sha ${SHACPPFLAGS}
.endfor
