#	$NetBSD: Makefile,v 1.5.2.2 2014/05/22 11:42:21 yamt Exp $
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
CPPFLAGS+=	-I${.CURDIR} -I.
DPADD+=		${LIBL}
LDADD+=		-ll -lutil -lc

COPTS.testlang_parse.c += -Wno-uninitialized
CWARNFLAGS.clang+=	-Wno-format -Wno-conversion

.include <bsd.test.mk>
