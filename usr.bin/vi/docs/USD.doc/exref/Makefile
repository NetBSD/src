#	$NetBSD: Makefile,v 1.4 2002/08/13 20:10:55 pooka Exp $
#
# @(#)Makefile	8.8 (Berkeley) 10/10/96

DIR=		usd/12.ex
SRCS=		ex.rm ex.summary
MACROS=		-ms
CLEANFILES=	summary.ps

all: paper.ps summary.ps

paper.ps: ex.rm
	${TBL} ex.rm | ${ROFF} ${MACROS} > ${.TARGET}

summary.ps: ex.summary
	${TBL} ex.summary | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
