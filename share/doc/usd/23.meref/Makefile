#	@(#)Makefile	6.4 (Berkeley) 6/30/90

DIR=	usd/23.meref
DOC=	23.meref

SRCS=	ref.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
