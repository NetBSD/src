#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	usd/33.rogue
DOC=	33.rogue

SRCS=	rogue.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
