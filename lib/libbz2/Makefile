#	$NetBSD: Makefile,v 1.3 2000/11/08 02:15:13 wiz Exp $

LIB=	bz2
MKMAN=	no

SRCS=blocksort.c huffman.c crctable.c randtable.c compress.c decompress.c bzlib.c
INCS=	bzlib.h
INCSDIR=/usr/include

TEXINFO= bzip2.info
CLEANFILES+=	bzip2.texi

bzip2.texi:   manual.texi
		@-rm -f $@
		sed -e 's/@parindent.*//' \
		    -e 's/@parskip.*//' \
		    -e 's/@node Overview, Implementation, Top, Top/@node Top, (dir), (dir), (dir)/' \
		    <$> >$@

.include <bsd.info.mk>
.include <bsd.lib.mk>
