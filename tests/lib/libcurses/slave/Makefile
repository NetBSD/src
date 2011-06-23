#	$NetBSD: Makefile,v 1.3.2.1 2011/06/23 14:20:40 cherry Exp $
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

CWARNFLAGS.clang+=	-Wno-format

.include <bsd.test.mk>
