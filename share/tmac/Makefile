#	@(#)Makefile	8.1 (Berkeley) 6/8/93
#	$NetBSD: Makefile,v 1.23 1999/07/10 20:04:03 christos Exp $

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
	@if [ ! -f ${.TARGET} -o ${.ALLSRC} -nt ${.TARGET} ]; then \
		echo "sed -f ${STRIP_SED} < ${.ALLSRC} > ${.TARGET}"; \
		sed -f ${STRIP_SED} < ${.ALLSRC} > ${.TARGET}; \
	fi
.endfor

.include <bsd.prog.mk>
