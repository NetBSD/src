#	from: @(#)Makefile	6.13 (Berkeley) 3/20/91
#	$NetBSD: Makefile,v 1.18 1997/05/09 07:50:25 mycroft Exp $

MSRCS=  andoc doc doc-ditroff doc-common doc-nroff doc-syms
MMACS=  ${MSRCS:S@^@tmac.@g}
FILES=	${MMACS}
FILESDIR=${BINDIR}/tmac
LINKS=	${FILESDIR}/tmac.andoc ${FILESDIR}/tmac.an
CLEANFILES+= ${MMACS}

STRIP_SED = ${.CURDIR}/../me/strip.sed

# old macros not currently installed...
OFILES=	tmac.an.v6compat tmac.an6n tmac.an6t tmac.a tmac.cp tmac.doc.old

all:	${MMACS}

.for i in ${MSRCS}
tmac.$i: $i
	sed -f ${STRIP_SED} < ${.ALLSRC} > ${.TARGET}
.endfor

.include <bsd.prog.mk>
