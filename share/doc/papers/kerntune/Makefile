#	$NetBSD: Makefile,v 1.2.12.1 2002/06/05 05:54:17 lukem Exp $
#
#	@(#)Makefile	1.5 (Berkeley) 6/8/93

DIR=	papers/kerntune
SRCS=	0.t 1.t 2.t 3.t 4.t
MACROS=	-ms

paper.ps: ${SRCS}
	${SOELIM} ${SRCS} | ${PIC} | ${TBL} | ${EQN} | \
	    ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
