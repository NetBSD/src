#	from: @(#)Makefile	8.1 (Berkeley) 8/14/93
#	$Id: Makefile,v 1.2 1994/01/24 06:39:51 cgd Exp $

DIR=	usd/12.vi
SRCS=	vi.in vi.chars
MACROS=	-msU
CLEANFILES+=summary.* viapwh.*

paper.ps: ${SRCS} summary.ps viapwh.ps
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}

summary.ps: vi.summary
	${TBL} vi.summary | ${ROFF}  > ${.TARGET}

viapwh.ps: vi.apwh.ms
	${ROFF} vi.apwh.ms > ${.TARGET}

.include <bsd.doc.mk>
