#	from: @(#)Makefile	6.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:36:59 cgd Exp $

DIR=	smm/02.config
DOC=	02.config

SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t a.t b.t c.t d.t e.t
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
