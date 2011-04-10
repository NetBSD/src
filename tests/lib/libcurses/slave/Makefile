#	$NetBSD: Makefile,v 1.1 2011/04/10 09:55:10 blymn Exp $
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
LDADD+=		-lcurses

.include <bsd.test.mk>
