#	$NetBSD: Makefile,v 1.5 2003/07/10 10:34:47 lukem Exp $
#
#	@(#)Makefile	8.7 (Berkeley) 8/18/96

DIR=		usd/12.vi
SRCS=		vi.in vi.chars
MACROS=		-ms
CLEANFILES+=	vitut.ps summary.ps viapwh.ps

all: vitut.ps summary.ps viapwh.ps

vitut.ps: ${SRCS}
	${TOOL_TBL} ${SRCS} | ${TOOL_ROFF_PS} ${MACROS} > $@
	chmod 444 $@

summary.ps: vi.summary
	${TOOL_TBL} vi.summary | ${TOOL_ROFF_PS} ${MACROS} > $@
	chmod 444 $@

viapwh.ps: vi.apwh.ms
	${TOOL_TBL} vi.apwh.ms | ${TOOL_ROFF_PS} ${MACROS} > $@
	chmod 444 $@

.include <bsd.doc.mk>
