#	$NetBSD: Makefile,v 1.2.12.1 2002/06/05 05:54:35 lukem Exp $
#
#	@(#)Makefile	1.4 (Berkeley) 6/8/93

DIR=	papers/newvm
SRCS=	0.t 1.t a.t
MACROS=	-ms

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
