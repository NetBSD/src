#	$NetBSD: Makefile.inc,v 1.3 2000/07/05 11:44:03 ad Exp $
#	$OpenBSD: Makefile.inc,v 1.5 1997/07/17 06:02:42 millert Exp $

# hash functions
.PATH: ${ARCHDIR}/hash ${.CURDIR}/hash

SRCS+=	rmd160.c rmd160l.c sha1.c sha1hl.c

MAN+=	sha1.3 rmd160.3

MLINKS+=sha1.3 SHA1Init.3    sha1.3 SHA1Update.3
MLINKS+=sha1.3 SHA1Final.3   sha1.3 SHA1Transform.3
MLINKS+=sha1.3 SHA1End.3     sha1.3 SHA1File.3
MLINKS+=sha1.3 SHA1Data.3

MLINKS+=rmd160.3 RMD160Init.3    rmd160.3 RMD160Update.3
MLINKS+=rmd160.3 RMD160Final.3   rmd160.3 RMD160Transform.3
MLINKS+=rmd160.3 RMD160End.3     rmd160.3 RMD160File.3
MLINKS+=rmd160.3 RMD160Data.3
