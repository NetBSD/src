#	@(#)Makefile	8.1 (Berkeley) 6/8/93
#	$NetBSD: Makefile,v 1.24 1999/08/30 00:41:57 christos Exp $

# IF YOU ADD TO MSRCS, MAKE SURE YOU UPDATE `CATDEPS' IN bsd.man.mk!
MSRCS=  andoc doc doc-ditroff doc-common doc-nroff doc-syms doc2html
MMACS=  ${MSRCS:S@^@tmac.@g}
FILES=	${MMACS}
FILESDIR=${BINDIR}/tmac
LINKS=	${FILESDIR}/tmac.andoc ${FILESDIR}/tmac.an
CLEANFILES+= ${MMACS}

TMACDIR?=${.CURDIR}
STRIP_SED = ${TMACDIR}/../me/strip.sed

# old macros not currently installed...
OFILES=	tmac.an.v6compat tmac.an6n tmac.an6t tmac.a tmac.cp tmac.doc.old

all:	${MMACS}

.for i in ${MSRCS}
tmac.$i: $i
	sed -f ${STRIP_SED} < ${.ALLSRC} > ${.TARGET}
.endfor

.include <bsd.prog.mk>
