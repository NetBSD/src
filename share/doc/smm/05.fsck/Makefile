#	from: @(#)Makefile	4.3 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:05 cgd Exp $

DIR=	smm/05.fsck
DOC=	05.fsck

SRCS=	0.t 1.t 2.t 3.t 4.t
TROFF=	ditroff

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${ROFF} ${SRCS} ) > ${.TARGET}
