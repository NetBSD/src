#	$NetBSD: Makefile,v 1.3 2003/01/05 12:34:27 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	usd/31.trek
SRCS=	trek.me
MACROS=	-me

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
