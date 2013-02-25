#	$NetBSD: Makefile,v 1.4.60.1 2013/02/25 00:27:45 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	usd/30.rogue
SRCS=	rogue.me
MACROS=	-me

paper.ps: ${SRCS}
	${TOOL_TBL} ${SRCS} | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
