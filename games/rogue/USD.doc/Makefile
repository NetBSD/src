#	$NetBSD: Makefile,v 1.3 2003/01/05 12:34:26 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	usd/30.rogue
SRCS=	rogue.me
MACROS=	-me

paper.${PRINTER}: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
