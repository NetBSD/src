#	$NetBSD: Makefile,v 1.3 2011/05/26 12:56:32 joerg Exp $
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
LDADD+=		-lcurses -lterminfo

CWARNFLAGS.clang+=	-Wno-format

.include <bsd.test.mk>
