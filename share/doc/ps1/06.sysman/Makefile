#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.6 1994/01/24 22:36:41 cgd Exp $

DIR=	ps1/06.sysman
DOC=	06.sysman

SRCS=	0.t 1.0.t 1.1.t 1.2.t 1.3.t 1.4.t 1.5.t 1.6.t 1.7.t \
	2.0.t 2.1.t 2.2.t 2.3.t 2.4.t 2.5.t a.t
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
