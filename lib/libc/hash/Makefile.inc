#	$NetBSD: Makefile.inc,v 1.1 1999/02/04 05:08:58 explorer Exp $
#	$OpenBSD: Makefile.inc,v 1.5 1997/07/17 06:02:42 millert Exp $

# hash functions
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/hash ${.CURDIR}/hash

SRCS+=	sha1.c sha1hl.c

MAN+=	sha1.3

MLINKS+=sha1.3 SHA1Init.3    sha1.3 SHA1Update.3
MLINKS+=sha1.3 SHA1Final.3   sha1.3 SHA1Transform.3
MLINKS+=sha1.3 SHA1End.3     sha1.3 SHA1File.3
MLINKS+=sha1.3 SHA1Data.3
