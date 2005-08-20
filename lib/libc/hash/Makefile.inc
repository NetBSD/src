#	$NetBSD: Makefile.inc,v 1.7 2005/08/20 18:57:50 elad Exp $
#	$OpenBSD: Makefile.inc,v 1.5 1997/07/17 06:02:42 millert Exp $

# hash functions
.PATH: ${ARCHDIR}/hash ${.CURDIR}/hash

SRCS+=	rmd160.c rmd160hl.c sha1.c sha1hl.c
SRCS+=	hmac_sha1.c hmac_md5.c

MAN+=	sha1.3 rmd160.3

MLINKS+=sha1.3 SHA1Init.3    sha1.3 SHA1Update.3
MLINKS+=sha1.3 SHA1Final.3   sha1.3 SHA1Transform.3
MLINKS+=sha1.3 SHA1End.3     sha1.3 SHA1File.3
MLINKS+=sha1.3 SHA1Data.3

MLINKS+=rmd160.3 RMD160Init.3    rmd160.3 RMD160Update.3
MLINKS+=rmd160.3 RMD160Final.3   rmd160.3 RMD160Transform.3
MLINKS+=rmd160.3 RMD160End.3     rmd160.3 RMD160File.3
MLINKS+=rmd160.3 RMD160Data.3

#.include "${.CURDIR}/hash/sha2/Makefile.inc"
