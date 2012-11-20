#	$NetBSD: Makefile,v 1.6.6.1 2012/11/20 23:26:54 matt Exp $
#
.include <bsd.own.mk>

ATFFILE=	no
TESTSDIR=	${TESTSBASE}/lib/libcurses
TESTS_C+=	slave

PROG=	slave
NOMAN=	"true"
#MAN=	slave.7
SRCS=	slave.c commands.c curses_commands.c

CPPFLAGS+=	-g -I${.CURDIR} -I. -I${.CURDIR}/../director
LDADD+=		-lcurses -lterminfo
.if ${HAVE_BINUTILS} == "219"
LDADD+=		-Wl,--no-fatal-warnings
.endif

COPTS.curses_commands.c += -Wno-format-nonliteral
CWARNFLAGS.clang+=	-Wno-format

.include <bsd.test.mk>
