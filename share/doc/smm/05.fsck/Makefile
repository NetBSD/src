#	@(#)Makefile	4.3 (Berkeley) 6/30/90

DIR=	smm/05.fsck
DOC=	05.fsck

SRCS=	0.t 1.t 2.t 3.t 4.t
TROFF=	ditroff

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
