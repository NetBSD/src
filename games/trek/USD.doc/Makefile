#	$NetBSD: Makefile,v 1.4 2003/07/10 10:34:20 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	usd/31.trek
SRCS=	trek.me
MACROS=	-me

paper.ps: ${SRCS}
	${TOOL_TBL} ${SRCS} | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
