#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/16.ex
DOC=	16.ex

SRCS=	ex.rm
MACROS=	-ms
CLEANFILES=summary.*

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS} ${DOC}.summary.${PRINTER}
	${ROFF} ${SRCS} > ${.TARGET}

${DOC}.summary.${PRINTER}: ex.summary
	${TBL} ex.summary | ${ROFF} > ${.TARGET}
