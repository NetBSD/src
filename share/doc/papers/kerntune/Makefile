#	$NetBSD: Makefile,v 1.4 2003/07/10 10:34:26 lukem Exp $
#
#	@(#)Makefile	1.5 (Berkeley) 6/8/93

DIR=	papers/kerntune
SRCS=	0.t 1.t 2.t 3.t 4.t
MACROS=	-ms

paper.ps: ${SRCS}
	${TOOL_SOELIM} ${SRCS} | ${TOOL_PIC} | ${TOOL_TBL} | ${TOOL_EQN} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
