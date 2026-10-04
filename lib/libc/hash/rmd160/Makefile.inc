# $NetBSD: Makefile.inc,v 1.2 2026/10/04 03:28:43 mrg Exp $

# hash functions
.PATH: ${.PARSEDIR}

SRCS+=	rmd160.c rmd160hl.c

MAN+=	rmd160.3

MLINKS+=rmd160.3 RMD160Init.3    rmd160.3 RMD160Update.3
MLINKS+=rmd160.3 RMD160Final.3   rmd160.3 RMD160Transform.3
MLINKS+=rmd160.3 RMD160End.3     rmd160.3 RMD160File.3
MLINKS+=rmd160.3 RMD160Data.3
