#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/10.etiq
DOC=	10.etiq

SRCS=	../../smm/10.newsop/tmac.n manner.mn

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
