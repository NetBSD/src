#	from: @(#)Makefile	5.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:25 cgd Exp $

DIR=	smm/16.sendmail
DOC=	16.sendmail

SRCS=	intro.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${PIC} ${SRCS} ) | ${ROFF} > ${.TARGET}
