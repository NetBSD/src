#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93
#	$Id: Makefile,v 1.2 1994/01/24 06:39:46 cgd Exp $

DIR=	usd/13.ex
SRCS=	ex.rm
MACROS=	-msU
CLEANFILES=summary.*

paper.ps: ${SRCS} summary.ps
	${ROFF} ${SRCS} > ${.TARGET}

summary.ps: ex.summary
	${TBL} ex.summary | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
