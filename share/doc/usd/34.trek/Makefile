#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/34.trek
DOC=	34.trek

SRCS=	trek.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}
