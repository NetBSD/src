#	$NetBSD: Makefile,v 1.4 2002/06/02 17:40:46 pooka Exp $
#
#	@(#)Makefile	1.6 (Berkeley) 6/8/93

DIR=	papers/relengr
SRCS=	0.t 1.t 2.t 3.t
MACROS=	-ms
EXTRA=	ref.bib tmac.srefs
REFER=	refer -n -e -l -s -p ref.bib

paper.ps: ${SRCS}
	${REFER} ${SRCS} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
