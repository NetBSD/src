#	$NetBSD: Makefile,v 1.3 2002/06/02 17:40:44 pooka Exp $
#
#	@(#)Makefile	6.3 (Berkeley) 6/8/93

DIR=	papers/diskperf
SRCS=	abs.ms motivation.ms equip.ms methodology.ms tests.ms results.ms \
	conclusions.ms appendix.ms
MACROS=	-ms

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
