# $NetBSD: Makefile,v 1.1 2011/04/10 09:55:08 blymn Exp $

NOMAN=		# defined

.include <bsd.own.mk>

SUBDIR=		director slave tests check_files

TESTSDIR=	${TESTSBASE}/lib/libcurses

TEST_TERMINFO=	atf.terminfo
TERMINFO_DB=	.terminfo
TERMINFODIR=	${TESTSDIR}

FILESDIR=	${TESTSDIR}
FILES=		${TERMINFO_DB}.db

TESTS_SH=	t_curses

HOME=		${TESTDIR}

CLEANFILES+=	${TERMINFO_DB}.db

realall: ${TERMINFO_DB}.db

${TERMINFO_DB}.db: ${TEST_TERMINFO}
	${TOOL_TIC} -o ${TERMINFO_DB} ${.CURDIR}/${TEST_TERMINFO}

.include <bsd.test.mk>
#.include <bsd.subdir.mk>
