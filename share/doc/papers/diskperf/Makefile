#	$NetBSD: Makefile,v 1.2 1998/01/09 06:40:55 perry Exp $
#
#	@(#)Makefile	6.3 (Berkeley) 6/8/93

DIR=	papers/diskperf
SRCS=	abs.ms motivation.ms equip.ms methodology.ms tests.ms results.ms \
	conclusions.ms appendix.ms
MACROS=	-ms

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
