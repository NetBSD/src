#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	smm/02.config
DOC=	02.config

SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t a.t b.t c.t d.t e.t
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
