#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/21.msdiffs
DOC=	21.msdiffs

SRCS=	ms.diffs
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
