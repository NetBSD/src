#	@(#)Makefile	5.2 (Berkeley) 6/30/90

DIR=	smm/10.newsop
DOC=	10.newsop

SRCS=	 tmac.n install.mn

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
