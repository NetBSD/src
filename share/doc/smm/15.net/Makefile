#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	smm/15.net
DOC=	15.net

SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t 7.t 8.t 9.t a.t b.t c.t d.t e.t f.t
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}
