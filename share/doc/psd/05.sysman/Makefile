#	$NetBSD: Makefile,v 1.7 2003/07/10 10:34:30 lukem Exp $
#
#	@(#)Makefile	8.2 (Berkeley) 6/1/94

DIR=	psd/05.sysman
SRCS=	0.t 1.0.t 1.1.t 1.2.t 1.3.t 1.4.t 1.5.t 1.6.t 1.7.t \
	2.0.t 2.1.t 2.2.t 2.3.t 2.4.t 2.5.t a.t
MACROS=	-ms
CLEANFILES+=Toc

paper.ps: ${SRCS}
	# Format twice to (only) generate Toc in the first run:
	( ${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_PS} ${MACROS} 2>&1 ) >/dev/null
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
