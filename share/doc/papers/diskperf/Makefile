#	$NetBSD: Makefile,v 1.2.12.1 2002/06/05 05:53:58 lukem Exp $
#
#	@(#)Makefile	6.3 (Berkeley) 6/8/93

DIR=	papers/diskperf
SRCS=	abs.ms motivation.ms equip.ms methodology.ms tests.ms results.ms \
	conclusions.ms appendix.ms
MACROS=	-ms

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
