#	$NetBSD: Makefile,v 1.5 2003/01/05 12:31:17 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 8/14/93

DIR=	usd/04.csh
SRCS=	tabs csh.1 csh.2 csh.3 csh.4 csh.a csh.g
MACROS= -ms

paper.ps: ${SRCS}
	${SOELIM} -I${.CURDIR} ${.ALLSRC} | ${ROFF} ${MACROS} > ${.TARGET}                  
.include <bsd.doc.mk>
