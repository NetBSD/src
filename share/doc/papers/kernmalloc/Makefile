#	$NetBSD: Makefile,v 1.2 1998/01/09 06:41:08 perry Exp $
#
#	@(#)Makefile	1.8 (Berkeley) 6/8/93

DIR=	papers/kernmalloc
SRCS=	kernmalloc.t appendix.t
MACROS=	-ms

paper.ps: ${SRCS} alloc.fig usage.tbl
	${SOELIM} ${SRCS} | ${TBL} | ${PIC} | ${EQN} | ${GRIND} | \
	    ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
