#	from: @(#)Makefile	5.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.5 1994/01/24 22:36:57 cgd Exp $

DIR=	smm/01.setup
DOC=	01.setup

SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t a.t b.t
.if ${MACHINE} == "vax"
SRCS+=	c.t
.endif
.PATH:	${.CURDIR}/${MACHINE} ${.CURDIR}/common

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${EQN} ${.ALLSRC} ) | ${TBL} | ${ROFF} > ${.TARGET}
