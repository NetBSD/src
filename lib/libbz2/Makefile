#	$NetBSD: Makefile,v 1.5 2001/06/03 13:42:00 simonb Exp $

LIB=	bz2
MKMAN=	no
DIST=	${.CURDIR}/../../dist/bzip2
.PATH:	${DIST}

SRCS=	blocksort.c huffman.c crctable.c randtable.c compress.c decompress.c \
	bzlib.c
INCS=	bzlib.h
INCSDIR=/usr/include

TEXINFO=	bzip2.info
CLEANFILES+=	bzip2.texi

bzip2.texi:   manual.texi
		@-rm -f $@
		sed -e 's/@parindent.*//' \
		    -e 's/@parskip.*//' \
		    -e 's/@node Overview, Implementation, Top, Top/@node Overview, (dir), Top, (dir)/' \
		    -e 's/@itemize$$/@itemize @bullet/' \
		    <$> >$@

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	${HTMLDOCDIR}/bzip2
FILES=		manual_toc.html manual_1.html manual_2.html manual_3.html \
		manual_4.html
.endif

.include <bsd.info.mk>
.include <bsd.lib.mk>
