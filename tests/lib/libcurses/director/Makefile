#	$NetBSD: Makefile,v 1.7 2014/01/19 21:50:17 apb Exp $
#
.include <bsd.own.mk>

ATFFILE=	no
TESTSDIR=       ${TESTSBASE}/lib/libcurses
TESTS_C+=	director

PROG=	director
NOMAN=	"true"
#MAN=	director.7
SRCS=	testlang_parse.y testlang_conf.l director.c

YHEADER=	1
CPPFLAGS+=	-O0 -I${.CURDIR} -I.
DPADD+=		${LIBL}
LDADD+=		-ll -lutil -lc

CWARNFLAGS.clang+=	-Wno-format -Wno-conversion

.include <bsd.test.mk>
