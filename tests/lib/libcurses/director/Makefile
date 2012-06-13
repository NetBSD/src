#	$NetBSD: Makefile,v 1.5.4.1 2012/06/13 18:55:24 riz Exp $
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
CPPFLAGS+=	-O0 -g -I${.CURDIR} -I.
LDFLAGS+=	-g
DPADD+=		${LIBL}
LDADD+=		-ll -lutil -lc

CWARNFLAGS.clang+=	-Wno-format -Wno-conversion

.include <bsd.test.mk>
