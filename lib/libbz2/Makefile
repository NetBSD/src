#	$NetBSD: Makefile,v 1.4 2000/11/10 02:00:10 wiz Exp $

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

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	${HTMLDOCDIR}/bzip2
FILES=		manual_toc.html manual_1.html manual_2.html manual_3.html \
		manual_4.html
.endif

.include <bsd.info.mk>
.include <bsd.lib.mk>
