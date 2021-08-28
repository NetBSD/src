# $NetBSD: Makefile,v 1.8 2021/08/28 19:45:18 rillig Exp $

NOMAN=		yes

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/xlint/lint2

TESTS_SH=	t_lint2

FILESDIR=	${TESTSDIR}

TESTS+=		emit
TESTS+=		emit_lp64
.for msg in \
    000 001 002 003 004 005 006 007 008 009 \
    010 011 012 013 014 015 016 017 018
TESTS+=		msg_${msg}
.endfor
TESTS+=		output_sorted
TESTS+=		read
TESTS+=		read_lp64
TESTS+=		read_printf

FILES+=		${TESTS:=.ln}
FILES+=		${TESTS:Nemit*:=.exp}
FILES+=		${TESTS:Memit*:=.exp-ln}

# Note: only works for adding tests.
# To remove a test, the $$mi file must be edited manually.
sync-mi: .PHONY
	@set -eu;							\
	cd "${MAKEFILE:tA:H}/../../../..";				\
	mi="distrib/sets/lists/tests/mi";				\
	cvs update "$$mi";						\
	fmt="./usr/tests/usr.bin/xlint/lint2/%s\ttests-usr.bin-tests\tcompattestfile,atf\n"; \
	cat "$$mi" > "$$mi.tmp";					\
	printf "$$fmt" ${FILES} >> "$$mi.tmp";				\
	distrib/sets/fmt-list "$$mi.tmp";				\
	mv "$$mi.tmp" "$$mi";						\
	cvs diff "$$mi" || true

.include <bsd.test.mk>
