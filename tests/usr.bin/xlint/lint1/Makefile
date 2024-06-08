# $NetBSD: Makefile,v 1.142 2024/06/08 09:09:20 rillig Exp $

NOMAN=		# defined

.include <bsd.own.mk>

ARCHSUBDIR!=	cd ${NETBSDSRCDIR}/usr.bin/xlint/lint1 && ${MAKE} -v ARCHSUBDIR

TESTSDIR=	${TESTSBASE}/usr.bin/xlint/lint1

TESTS_SH=	t_integration t_usage
TESTS_SH_SRC_t_integration=	archsubdir.sh t_integration.sh
CLEANFILES+=	archsubdir.sh
CLEANFILES+=	*.exp

archsubdir.sh:
	@echo archsubdir=${ARCHSUBDIR} >${.TARGET}

FILESDIR=	${TESTSDIR}
FILES+=		${:!cd ${.CURDIR} && echo *.c *.exp-ln!}
FILES+=		check-expect.lua

add-test: .PHONY
	@set -eu; \
	test=${NAME:Q}; \
	[ "$$test" ] || { \
		echo "usage: ${MAKE} add-test NAME=<name>"; \
		exit; \
	}; \
	\
	if [ -f "$$test.c" ]; then \
		echo "error: test $$test already exists in $$PWD."; \
		exit 1; \
	fi; \
	\
	echo "=> Adding test $$test"; \
	printf '%s\n' \
		'/*	$$''NetBSD$$	*/' \
		"# 3 \"$$test.c\"" \
		'' \
		'/*' \
		' * TODO: Explain the purpose of the test.' \
		' */' \
		'' \
		'/* lint1-extra-flags: -Z */' \
		'/* lint1-flags: -Z */' \
		'/* lint1-only-if: schar uchar ilp32 lp64 int long ldbl64 ldbl96 ldbl128 */' \
		'' \
		'// TODO: Add some code that passes.' \
		'// TODO: Add some code that fails.' \
	> "$$test.c"; \
	cvs add "$$test.c"; \
	${MAKE} sync-mi

# Note: only works for adding tests.
# To remove a test, the $$mi file must be edited manually.
sync-mi: .PHONY
	@set -eu;							\
	cd "${MAKEFILE:tA:H}/../../../..";				\
	mi="distrib/sets/lists/tests/mi";				\
	cvs update "$$mi";						\
	fmt="./usr/tests/usr.bin/xlint/lint1/%s\ttests-usr.bin-tests\tcompattestfile,atf\n"; \
	cat "$$mi" > "$$mi.tmp";					\
	printf "$$fmt" ${FILES} >> "$$mi.tmp";				\
	distrib/sets/fmt-list "$$mi.tmp";				\
	mv "$$mi.tmp" "$$mi";						\
	cvs diff "$$mi" || true

accept: .PHONY
	@archsubdir=${ARCHSUBDIR:Q} sh ./accept.sh ''

.include <bsd.test.mk>
