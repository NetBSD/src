#	$NetBSD: Makefile,v 1.8 1997/10/25 10:11:22 lukem Exp $

PROG=	grep
SRCS=	dfa.c grep.c getopt.c kwset.c obstack.c regex.c search.c
CPPFLAGS+=-DGREP -DHAVE_STRING_H=1 -DHAVE_SYS_PARAM_H=1 -DHAVE_UNISTD_H=1 \
	-DHAVE_GETPAGESIZE=1 -DHAVE_MEMCHR=1 -DHAVE_STRERROR=1 \
	-DHAVE_VALLOC=1

LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}

LINKS+= ${BINDIR}/grep ${BINDIR}/egrep \
	${BINDIR}/grep ${BINDIR}/fgrep
MLINKS= grep.1 egrep.1 grep.1 fgrep.1

check:	all
	sh ${.CURDIR}/tests/check.sh ${.CURDIR}/tests

.include <bsd.prog.mk>
