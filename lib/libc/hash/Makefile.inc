#	$NetBSD: Makefile.inc,v 1.13 2016/07/01 22:41:39 christos Exp $
#	$OpenBSD: Makefile.inc,v 1.5 1997/07/17 06:02:42 millert Exp $

# hash functions
.PATH: ${ARCHDIR}/hash ${.CURDIR}/hash

SRCS+= hmac.c
MAN+= hmac.3

.include "${.CURDIR}/hash/md2/Makefile.inc"
.include "${.CURDIR}/hash/rmd160/Makefile.inc"
.include "${.CURDIR}/hash/sha1/Makefile.inc"
.include "${.CURDIR}/hash/sha2/Makefile.inc"

.include "${.CURDIR}/hash/murmurhash/Makefile.inc"
