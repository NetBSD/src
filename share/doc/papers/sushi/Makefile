#	$NetBSD: Makefile,v 1.1 2004/03/09 20:47:40 garbled Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 4/20/94

DIR=	papers/sushi
SRCS=	sushi.me
MACROS=	-me

paper.ps: ${SRCS}
	${TOOL_PIC} ${SRCS} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
