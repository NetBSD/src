#	from: @(#)Makefile	6.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1993/10/23 00:24:50 cgd Exp $

DIR=	usd/15.vi
DOC=	15.vi

SRCS=	vi.in vi.chars
MACROS=	-ms
CLEANFILES+=summary.* viapwh.*

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS} ${DOC}.summary.${PRINTER} ${DOC}.viapwh.${PRINTER}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}

${DOC}.summary.${PRINTER}: vi.summary
	( cd ${.CURDIR} ; ${TBL} vi.summary ) | ${ROFF}  > ${.TARGET}

${DOC}.viapwh.${PRINTER}: vi.apwh.ms
	( cd ${.CURDIR} ; ${ROFF} vi.apwh.ms ) > ${.TARGET}
