#	from: @(#)Makefile	1.4 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.6 1994/01/24 22:36:47 cgd Exp $

DIR=	ps1/08.ipc
DOC=	08.ipc

SRCS=	0.t 1.t 2.t 3.t 4.t 5.t
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}
