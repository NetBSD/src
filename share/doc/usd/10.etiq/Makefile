#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.6 1994/01/24 22:37:40 cgd Exp $

DIR=	usd/10.etiq
DOC=	10.etiq

SRCS=	../../smm/10.newsop/tmac.n manner.mn

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
