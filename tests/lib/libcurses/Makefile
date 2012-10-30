# $NetBSD: Makefile,v 1.2.2.1 2012/10/30 19:00:02 yamt Exp $

NOMAN=		# defined

.include <bsd.own.mk>

SUBDIR=		director slave tests check_files

TESTSDIR=	${TESTSBASE}/lib/libcurses

TEST_TERMINFO=	atf.terminfo
TERMINFO_DB=	terminfo
TERMINFODIR=	${TESTSDIR}

FILESDIR=	${TESTSDIR}
FILES=		${TERMINFO_DB}.cdb

TESTS_SH=	t_curses

HOME=		${TESTDIR}

CLEANFILES+=	${TERMINFO_DB}.cdb

realall: ${TERMINFO_DB}.cdb

${TERMINFO_DB}.cdb: ${TOOL_TIC} ${TEST_TERMINFO}
	${TOOL_TIC} -o ${.TARGET} ${.CURDIR}/${TEST_TERMINFO}

.include <bsd.test.mk>
#.include <bsd.subdir.mk>
