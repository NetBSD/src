#	$NetBSD: Makefile,v 1.2 2011/04/13 09:46:38 he Exp $
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

.include <bsd.test.mk>
