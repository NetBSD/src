#	from: @(#)Makefile	6.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:56 cgd Exp $

DIR=	usd/33.rogue
DOC=	33.rogue

SRCS=	rogue.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
