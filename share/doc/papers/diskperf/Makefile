#	$NetBSD: Makefile,v 1.4 2003/07/10 10:34:26 lukem Exp $
#
#	@(#)Makefile	6.3 (Berkeley) 6/8/93

DIR=	papers/diskperf
SRCS=	abs.ms motivation.ms equip.ms methodology.ms tests.ms results.ms \
	conclusions.ms appendix.ms
MACROS=	-ms

paper.ps: ${SRCS}
	${TOOL_TBL} ${SRCS} | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
