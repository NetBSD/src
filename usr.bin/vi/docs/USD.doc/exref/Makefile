#	$NetBSD: Makefile,v 1.3 2001/03/31 11:37:47 aymeric Exp $
#
# @(#)Makefile	8.8 (Berkeley) 10/10/96

DIR=		usd/12.ex
SRCS=		ex.rm ex.summary
MACROS=		-ms
CLEANFILES=	summary.ps

all: paper.ps summary.ps

paper.ps: ex.rm
	${TBL} ex.rm | ${ROFF} > ${.TARGET}

summary.ps: ex.summary
	${TBL} ex.summary | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
