#	$NetBSD: Makefile.inc,v 1.14.16.1 2026/11/04 23:19:18 perseant Exp $
#	$OpenBSD: Makefile.inc,v 1.5 1997/07/17 06:02:42 millert Exp $

# hash functions
.PATH: ${ARCHDIR}/hash ${.PARSEDIR}

SRCS+= hmac.c
MAN+= hmac.3

.include "${.PARSEDIR}/md2/Makefile.inc"
.include "${.PARSEDIR}/rmd160/Makefile.inc"
.include "${.PARSEDIR}/sha1/Makefile.inc"
.include "${.PARSEDIR}/sha2/Makefile.inc"
.include "${.PARSEDIR}/sha3/Makefile.inc"
.include "${.PARSEDIR}/murmurhash/Makefile.inc"
