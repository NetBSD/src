#	$NetBSD: Makefile,v 1.5 2003/07/10 10:34:46 lukem Exp $
#
# @(#)Makefile	8.8 (Berkeley) 10/10/96

DIR=		usd/12.ex
SRCS=		ex.rm ex.summary
MACROS=		-ms
CLEANFILES=	summary.ps

all: paper.ps summary.ps

paper.ps: ex.rm
	${TOOL_TBL} ex.rm | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

summary.ps: ex.summary
	${TOOL_TBL} ex.summary | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
