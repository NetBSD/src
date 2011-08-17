#	$NetBSD: Makefile,v 1.5 2011/08/17 16:57:13 christos Exp $
#
.include <bsd.own.mk>

NOATFFILE=	true
TESTSDIR=	${TESTSBASE}/lib/libcurses
TESTS_C+=	slave

PROG=	slave
NOMAN=	"true"
#MAN=	slave.7
SRCS=	slave.c commands.c curses_commands.c

CPPFLAGS+=	-g -I${.CURDIR} -I. -I${.CURDIR}/../director
LDADD+=		-lcurses -lterminfo -Wl,--no-fatal-warnings

COPTS.curses_commands.c += -Wno-format-nonliteral
CWARNFLAGS.clang+=	-Wno-format

.include <bsd.test.mk>
