#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:48 cgd Exp $

DIR=	usd/16.ex
DOC=	16.ex

SRCS=	ex.rm
MACROS=	-ms
CLEANFILES=summary.*

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS} ${DOC}.summary.${PRINTER_TYPE}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}

${DOC}.summary.${PRINTER_TYPE}: ex.summary
	( cd ${.CURDIR} ; ${TBL} ex.summary ) | ${ROFF} > ${.TARGET}
