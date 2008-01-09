#	$NetBSD: Makefile,v 1.1.4.2 2008/01/09 01:38:55 matt Exp $

DIR=	usd/17.msmacros
SRCS=	ms fig1.pic
MACROS=	-ms

paper.ps: ${SRCS}
	${TOOL_SOELIM} ${SRCS} | ${TOOL_PIC} | ${TOOL_TBL} | ${TOOL_EQN} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
