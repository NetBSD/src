#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:50 cgd Exp $

DIR=	usd/21.msdiffs
DOC=	21.msdiffs

SRCS=	ms.diffs
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
