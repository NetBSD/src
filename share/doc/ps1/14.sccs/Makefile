#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:36:51 cgd Exp $

DIR=	ps1/14.sccs
DOC=	14.sccs

SRCS=	sccs.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
