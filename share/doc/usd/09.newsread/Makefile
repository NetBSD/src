#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.6 1994/01/24 22:37:38 cgd Exp $

DIR=	usd/09.newsread
DOC=	09.newsread

SRCS=	../../smm/10.newsop/tmac.n howto.mn

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
