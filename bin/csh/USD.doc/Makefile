#	$NetBSD: Makefile,v 1.6 2003/07/10 10:34:00 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 8/14/93

DIR=	usd/04.csh
SRCS=	tabs csh.1 csh.2 csh.3 csh.4 csh.a csh.g
MACROS= -ms

paper.ps: ${SRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}                  

.include <bsd.doc.mk>
