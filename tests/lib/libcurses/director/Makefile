#	$NetSD$
#
.include <bsd.own.mk>

NOATFFILE=	true
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

.include <bsd.test.mk>
