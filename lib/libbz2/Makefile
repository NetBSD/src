#	$NetBSD: Makefile,v 1.8 2002/06/27 21:03:37 thorpej Exp $

.include <bsd.own.mk>

LIB=		bz2
NOMAN=		# defined
DIST=		${.CURDIR}/../../dist/bzip2
.PATH:		${DIST}

SRCS=		blocksort.c huffman.c crctable.c randtable.c compress.c \
		decompress.c bzlib.c
INCS=		bzlib.h
INCSDIR=	/usr/include

TEXINFO=	bzip2.info
CLEANFILES+=	bzip2.texi

# XXX huffman.c gets mis-compiled with 2.95.3
.if ${MACHINE_ARCH} == "vax"
COPTS+=		-O0
.endif

bzip2.texi: manual.texi
	@-rm -f $@
	sed -e 's/@parindent.*//' \
	    -e 's/@parskip.*//' \
	    -e 's/@node Overview, Implementation, Top, Top/@node Overview, (dir), Top, (dir)/' \
	    -e 's/@itemize$$/@itemize @bullet/' \
	    -e 's/@bf{\(.*\)}/\1/' \
	    <$> >$@

.if ${MKSHARE} != "no"
FILESDIR=	${HTMLDOCDIR}/bzip2
FILES=		manual_toc.html manual_1.html manual_2.html manual_3.html \
		manual_4.html
.endif

.include <bsd.info.mk>
.include <bsd.lib.mk>
