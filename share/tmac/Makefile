#	@(#)Makefile	8.1 (Berkeley) 6/8/93
#	$NetBSD: Makefile,v 1.30 2004/03/29 17:08:25 wiz Exp $

# IF YOU ADD TO MSRCS, MAKE SURE YOU UPDATE `CATDEPS' IN bsd.man.mk!
MSRCS=	doc2html
MMACS=  ${MSRCS:S@$@.tmac@g}
INCS=	${MMACS}
INCSDIR=${BINDIR}/tmac
CLEANFILES+= ${MMACS}

TMACDIR?=${.CURDIR}
STRIP_SED = ${TMACDIR}/../me/strip.sed

# old macros not currently installed...
OFILES=	tmac.an.v6compat tmac.an6n tmac.an6t tmac.a tmac.cp tmac.doc.old

realall:	${MMACS}

.for i in ${MSRCS}
$i.tmac: $i
	${_MKTARGET_CREATE}
	sed -f ${STRIP_SED} < ${.ALLSRC} > ${.TARGET}
.endfor

.include <bsd.prog.mk>
