#	@(#)Makefile	5.3 (Berkeley) 6/30/90

DIR=	smm/16.sendmail
DOC=	16.sendmail

SRCS=	intro.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER}: ${SRCS}
	${PIC} ${SRCS} | ${ROFF} > ${.TARGET}
