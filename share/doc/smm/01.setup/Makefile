#	$NetBSD: Makefile,v 1.10.12.1 2002/06/05 05:55:59 lukem Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 7/27/93

DIR=	smm/01.setup
SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t
FILES=	${SRCS}
MACROS=	-ms

paper.ps: ${SRCS}
	${SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TBL} | \
	    ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
