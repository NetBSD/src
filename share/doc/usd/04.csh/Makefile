#	from: @(#)Makefile	5.1 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:33 cgd Exp $

DIR=	usd/04.csh
DOC=	04.csh

SRCS=	tabs csh.1 csh.2 csh.3 csh.4 csh.a csh.g

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
