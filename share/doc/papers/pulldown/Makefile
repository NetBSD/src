#	$NetBSD: Makefile,v 1.4 2004/06/30 03:26:27 jmc Exp $

DIR=	papers/pulldown
SRCS=	0.t 1.t 2.t 4.t 8.t 9.t
MACROS=	-ms
DPSRCS=	${SRCS} refs.r Makefile

paper.ps: ${DPSRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${TOOL_REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${TOOL_PIC} | ${TOOL_TBL} | ${TOOL_EQN} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

paper.dvi: ${DPSRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${TOOL_REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${TOOL_PIC} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_DVI} ${MACROS} > ${.TARGET}

paper.txt: ${DPSRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${SRCS} | \
	    ${TOOL_REFER} -P -S -e -p ${.CURDIR}/refs.r | \
	    ${TOOL_PIC} | ${TOOL_TBL} | ${TOOL_EQN} -Tascii | \
	    ${TOOL_ROFF_ASCII} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
