#	from: @(#)Makefile	5.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:14 cgd Exp $

DIR=	smm/10.newsop
DOC=	10.newsop

SRCS=	 tmac.n install.mn

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
