#	$NetBSD: Makefile,v 1.11.4.1 2008/01/09 01:33:52 matt Exp $

USE_FORT?= yes	# data driven bugs?

NOMAN=		# defined

.include <bsd.own.mk>

LIB=		bz2
DIST=		${NETBSDSRCDIR}/dist/bzip2
.PATH:		${DIST}

SRCS=		blocksort.c huffman.c crctable.c randtable.c compress.c \
		decompress.c bzlib.c
INCS=		bzlib.h
INCSDIR=	/usr/include

TEXINFO=	bzip2.info
CLEANFILES+=	bzip2.texi

# XXX huffman.c gets mis-compiled with 2.95.3
.if ${MACHINE_ARCH} == "vax" && ${HAVE_GCC} < 3
COPTS+=		-O0
.endif

# XXX blocksort.c gets mis-compiled with 4.1
.if (${MACHINE_ARCH} == "sh3el" || ${MACHINE_ARCH} == "sh3eb") && \
    ${HAVE_GCC} == 4
COPTS.blocksort.c+=		-O0
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
