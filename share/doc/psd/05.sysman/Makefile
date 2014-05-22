#	$NetBSD: Makefile,v 1.7.54.1 2014/05/22 11:37:45 yamt Exp $
#
#	@(#)Makefile	8.2 (Berkeley) 6/1/94

DIR=	psd/05.sysman
SRCS=	0.t 1.0.t 1.1.t 1.2.t 1.3.t 1.4.t 1.5.t 1.6.t 1.7.t \
	2.0.t 2.1.t 2.2.t 2.3.t 2.4.t 2.5.t a.t
MACROS=	-ms
CLEANFILES+=facilities contents pagelog

paper.ps: ${SRCS} facilities contents
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC:M*.t} | ${TOOL_TBL} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

facilities: ${SRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} |\
	    ${TOOL_SED} -n -e '/^\.Fd/p;/^\.Sh/p' |\
	    ${TOOL_SED} -e 's/^\.Fd/.Nm/;s/^\.Sh /.L/' > ${.TARGET}

contents: ${SRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} |\
	    ${TOOL_SED} -n -e '/^\.Sh/p' |\
	    ${TOOL_SED} -e 's/^\.Sh /.L/;s/$$/"/' > ${.TARGET}

.include <bsd.doc.mk>
