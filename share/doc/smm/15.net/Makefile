#	from: @(#)Makefile	6.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:23 cgd Exp $

DIR=	smm/15.net
DOC=	15.net

SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t 7.t 8.t 9.t a.t b.t c.t d.t e.t f.t
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
