#	$NetBSD: Makefile,v 1.4 2003/07/10 10:34:46 lukem Exp $
#
# @(#)Makefile	8.7 (Berkeley) 8/18/96

all: vi.0 vi.0.ps

vi.0: vi.1
	${TOOL_ROFF_ASCII} -man < vi.1 > $@
	chmod 444 $@

vi.0.ps: vi.1
	${TOOL_ROFF_PS} -man < vi.1 > $@
	chmod 444 $@

clean:
	rm -f vi.0 vi.0.ps

.include <bsd.doc.mk>
