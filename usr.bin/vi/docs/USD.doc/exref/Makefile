#	$NetBSD: Makefile,v 1.2 1998/01/09 08:07:17 perry Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	usd/13.ex
SRCS=	ex.rm
MACROS=	-msU
CLEANFILES=summary.*

paper.ps: ${SRCS} summary.ps
	${ROFF} ${SRCS} > ${.TARGET}

summary.ps: ex.summary
	${TBL} ex.summary | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
