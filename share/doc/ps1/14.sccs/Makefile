#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	ps1/14.sccs
DOC=	14.sccs

SRCS=	sccs.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
