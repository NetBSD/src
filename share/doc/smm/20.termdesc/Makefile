#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:27 cgd Exp $

DIR=	smm/20.termdesc
DOC=	20.termdesc

SRCS=	termdesc.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
