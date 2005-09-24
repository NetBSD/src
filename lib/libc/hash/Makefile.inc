#	$NetBSD: Makefile.inc,v 1.9 2005/09/24 19:04:52 elad Exp $
#	$OpenBSD: Makefile.inc,v 1.5 1997/07/17 06:02:42 millert Exp $

# hash functions
.PATH: ${ARCHDIR}/hash ${.CURDIR}/hash

.include "${.CURDIR}/hash/md5/Makefile.inc"
.include "${.CURDIR}/hash/rmd160/Makefile.inc"
.include "${.CURDIR}/hash/sha1/Makefile.inc"
.include "${.CURDIR}/hash/sha2/Makefile.inc"

