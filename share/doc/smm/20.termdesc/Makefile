#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	smm/20.termdesc
DOC=	20.termdesc

SRCS=	termdesc.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
