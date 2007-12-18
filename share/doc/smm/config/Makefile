#	$NetBSD: Makefile,v 1.1 2007/12/18 03:35:53 garbled Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 7/27/93

DIR=	smm/config
SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t a.t b.t c.t d.t e.t
FILES=	${SRCS}
MACROS=	-ms

paper.ps: ${SRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
