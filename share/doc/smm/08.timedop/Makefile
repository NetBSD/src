#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	smm/08.timedop
DOC=	08.timedop

SRCS=	timed.ms
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
