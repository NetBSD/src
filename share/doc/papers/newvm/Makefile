#	$NetBSD: Makefile,v 1.2 1998/01/09 06:41:38 perry Exp $
#
#	@(#)Makefile	1.4 (Berkeley) 6/8/93

DIR=	papers/newvm
SRCS=	0.t 1.t a.t
MACROS=	-ms

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
