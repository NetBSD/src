#	from: @(#)Makefile	6.13 (Berkeley) 3/20/91
#	$NetBSD: Makefile,v 1.15 1997/03/24 21:57:13 christos Exp $

BINDIR= /usr/share
MSRCS=  doc doc-ditroff doc-common doc-nroff doc-syms
MMACS=  ${MSRCS:S@^@tmac.@g}
FILES=	tmac.andoc ${MMACS}
FILESDIR=${BINDIR}/tmac
LINKS=	${BINDIR}/tmac/tmac.andoc ${BINDIR}/tmac/tmac.an
CLEANFILES+= ${MMACS}

STRIP_SED = ${.CURDIR}/../me/strip.sed

# old macros not currently installed...
OFILES=	tmac.an.v6compat tmac.an6n tmac.an6t tmac.a tmac.cp tmac.doc.old

all:	${MMACS}

depend lint tags:

.for i in ${MSRCS}
tmac.$i: $i
	sed -f ${STRIP_SED} < ${.ALLSRC} > ${.TARGET}
.endfor

.include <bsd.prog.mk>
