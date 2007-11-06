#	$NetBSD: Makefile,v 1.6.22.1 2007/11/06 23:07:16 matt Exp $
#	@(#)Makefile	8.1 (Berkeley) 8/14/93

DIR=	usd/04.csh
SRCS=	tabs csh.1 csh.2 csh.3 csh.4 csh.ap csh.g
MACROS= -ms

paper.ps: ${SRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}                  

.include <bsd.doc.mk>
