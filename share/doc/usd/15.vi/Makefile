#	from: @(#)Makefile	6.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.7 1994/04/29 00:05:49 cgd Exp $

DIR=	usd/15.vi
DOC=	15.vi

SRCS=	vi.in vi.chars
EXTRA=	vi.summary vi.apwh.ms
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS} ${DOC}.summary.${PRINTER_TYPE} ${DOC}.viapwh.${PRINTER_TYPE}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}

${DOC}.summary.${PRINTER_TYPE}: vi.summary
	( cd ${.CURDIR} ; ${TBL} vi.summary ) | ${ROFF}  > ${.TARGET}

${DOC}.viapwh.${PRINTER_TYPE}: vi.apwh.ms
	( cd ${.CURDIR} ; ${ROFF} vi.apwh.ms ) > ${.TARGET}
