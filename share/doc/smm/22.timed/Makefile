#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	smm/22.timed
DOC=	22.timed

SRCS=	timed.ms
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	( cd ${.CURDIR} ; ${SOELIM} ${SRCS} ) | ${TBL} | ${ROFF} > ${.TARGET}
