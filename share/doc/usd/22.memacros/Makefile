#	from: @(#)Makefile	6.4 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:52 cgd Exp $

DIR=	usd/22.memacros
DOC=	22.memacros

SRCS=	intro.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
