# $NetBSD: Makefile.inc,v 1.2.84.1 2019/06/10 22:05:19 christos Exp $

# hash functions
.PATH: ${.CURDIR}/hash/sha1

SRCS+=	sha1.c sha1hl.c

MAN+=	sha1.3

MLINKS+=sha1.3 SHA1Init.3 sha1.3 SHA1Update.3 sha1.3 SHA1Final.3
MLINKS+=sha1.3 SHA1End.3  sha1.3 SHA1File.3   sha1.3 SHA1Data.3
MLINKS+=sha1.3 SHA1Transform.3  sha1.3 SHA1FileChunk.3
