#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.6 1994/01/24 22:36:49 cgd Exp $

DIR=	ps1/13.rcs
DOC=	13.rcs

SRCS=	rcs.ms
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
