#	from: @(#)Makefile	6.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:12 cgd Exp $

DIR=	smm/08.timedop
DOC=	08.timedop

SRCS=	timed.ms
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
