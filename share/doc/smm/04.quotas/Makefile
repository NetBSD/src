#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	smm/04.quotas
DOC=	04.quotas

SRCS=	quotas.ms
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
