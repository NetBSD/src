#	from: @(#)Makefile	5.4 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:20 cgd Exp $

DIR=	smm/14.fastfs
DOC=	14.fastfs

SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t
MACROS=	-ms

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${EQN} | ${ROFF} > ${.TARGET}
