#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	ps1/13.rcs
DOC=	13.rcs

SRCS=	rcs.ms
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
