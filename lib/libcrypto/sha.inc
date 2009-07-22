#	$NetBSD: sha.inc,v 1.8.4.1 2009/07/22 22:31:07 snj Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/sha/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/sha


SHA_SRCS = sha_dgst.c sha1dgst.c sha_one.c sha1_one.c

# Replaced OpenSSL version to avoid overlap with libc
SHA_SRCS+= libc-sha512.c libc-sha256.c

SRCS += ${SHA_SRCS}

.for cryptosrc in ${SHA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/sha
.endfor
