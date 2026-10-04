# $NetBSD: Makefile.inc,v 1.4 2026/10/04 03:28:43 mrg Exp $

# hash functions
.PATH: ${.PARSEDIR}

SRCS+=	sha1.c sha1hl.c

MAN+=	sha1.3

MLINKS+=sha1.3 SHA1Init.3 sha1.3 SHA1Update.3 sha1.3 SHA1Final.3
MLINKS+=sha1.3 SHA1End.3  sha1.3 SHA1File.3   sha1.3 SHA1Data.3
MLINKS+=sha1.3 SHA1Transform.3  sha1.3 SHA1FileChunk.3
