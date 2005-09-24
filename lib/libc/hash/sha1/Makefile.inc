# $NetBSD: Makefile.inc,v 1.1 2005/09/24 19:04:52 elad Exp $

# hash functions
.PATH: ${.CURDIR}/hash/sha1

SRCS+=	sha1.c sha1hl.c hmac_sha1.c

MAN+=	sha1.3

MLINKS+=sha1.3 SHA1Init.3 sha1.3 SHA1Update.3 sha1.3 SHA1Final.3
MLINKS+=sha1.3 SHA1End.3  sha2.3 SHA1File.3   sha1.3 SHA1Data.3
MLINKS+=sha1.3 SHA1Transform.3  sha1.3 SHA1FileChunk.3
