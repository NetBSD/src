#	@(#)Makefile	6.4 (Berkeley) 6/30/90

DIR=	usd/22.memacros
DOC=	22.memacros

SRCS=	intro.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
