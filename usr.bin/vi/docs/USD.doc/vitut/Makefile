#	$NetBSD: Makefile,v 1.3 2001/03/31 11:37:48 aymeric Exp $
#
#	@(#)Makefile	8.7 (Berkeley) 8/18/96

DIR=		usd/12.vi
SRCS=		vi.in vi.chars
MACROS=		-ms
CLEANFILES+=	vitut.ps summary.ps viapwh.ps

all: vitut.ps summary.ps viapwh.ps

vitut.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > $@
	chmod 444 $@

summary.ps: vi.summary
	${TBL} vi.summary | ${ROFF} > $@
	chmod 444 $@

viapwh.ps: vi.apwh.ms
	${TBL} vi.apwh.ms | ${ROFF} > $@
	chmod 444 $@

.include <bsd.doc.mk>
