#	from: @(#)Makefile	5.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:37:09 cgd Exp $

DIR=	smm/07.sendmailop
DOC=	07.sendmailop

SRCS=	op.me
MACROS=	-me

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${PIC} ${SRCS} ) | ${EQN} | ${ROFF} > ${.TARGET}
