#	$NetBSD: Makefile,v 1.2.12.1 2002/06/05 05:54:07 lukem Exp $
#
#	@(#)Makefile	1.8 (Berkeley) 6/8/93

DIR=	papers/kernmalloc
SRCS=	kernmalloc.t appendix.t
MACROS=	-ms

paper.ps: ${SRCS} alloc.fig usage.tbl
	${SOELIM} ${SRCS} | ${TBL} | ${PIC} | ${EQN} | ${GRIND} | \
	    ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
