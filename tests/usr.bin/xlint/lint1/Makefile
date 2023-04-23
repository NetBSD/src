# $NetBSD: Makefile,v 1.141 2023/04/23 08:47:27 rillig Exp $

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
