#	from: @(#)Makefile	6.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:07 cgd Exp $

DIR=	smm/06.lpd
DOC=	06.lpd

SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t 7.t
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
