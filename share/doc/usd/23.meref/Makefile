#	from: @(#)Makefile	6.4 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:54 cgd Exp $

DIR=	usd/23.meref
DOC=	23.meref

SRCS=	ref.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
