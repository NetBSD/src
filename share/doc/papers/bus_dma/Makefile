#	$NetBSD: Makefile,v 1.6 2003/07/10 10:34:25 lukem Exp $

DIR=	papers/bus_dma
SRCS=	title.me 1.me 2.me 3.me 4.me 5.me about.me
EXTRA=	figure1.pic figure2.pic figure3.pic
MACROS=	-me
DPSRCS=	${SRCS} ${EXTRA} Makefile

paper.ps: ${DPSRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${SRCS} | ${TOOL_PIC} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

paper.dvi: ${DPSRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${SRCS} | ${TOOL_PIC} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_DVI} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
