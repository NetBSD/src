#	$NetBSD: Makefile,v 1.5 2003/07/10 10:34:32 lukem Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/10/93

DIR=	smm/18.net
SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t 7.t 8.t 9.t a.t b.t c.t d.t e.t f.t
MACROS=	-ms

paper.ps: ${SRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
