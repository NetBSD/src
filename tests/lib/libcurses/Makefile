# $NetBSD: Makefile,v 1.4 2020/02/07 22:05:16 uwe Exp $

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

.if ${USETOOLS} == "yes"
DPTOOL_TIC = ${TOOL_TIC} 
.endif

${TERMINFO_DB}.cdb: ${DPTOOL_TIC} ${TEST_TERMINFO}
	${TOOL_TIC} -o ${.TARGET} ${.CURDIR}/${TEST_TERMINFO}

.include <bsd.test.mk>
#.include <bsd.subdir.mk>
