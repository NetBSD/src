#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	usd/15.vi
DIR=	15.vi

SRCS=	vi.in vi.chars
MACROS=	-ms
CLEANFILES+=summary.* viapwh.*

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS} ${DOC}.summary.${PRINTER} ${DOC}.viapwh.${PRINTER}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}

${DOC}.summary.${PRINTER}: vi.summary
	${TBL} vi.summary | ${ROFF}  > ${.TARGET}

${DOC}.viapwh.${PRINTER}: vi.apwh.ms
	${ROFF} vi.apwh.ms > ${.TARGET}
