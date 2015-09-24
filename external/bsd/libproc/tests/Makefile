# $NetBSD: Makefile,v 1.1 2015/09/24 14:12:48 christos Exp $

NOMAN=yes
.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libproc

.PATH: ${.CURDIR}/../dist/tests

TESTS_C+=	proc_test

BINDIR=         ${TESTSDIR}
PROGS=		target_prog

LDADD+=	-lelf -lproc -lrtld_db -lutil
DPADD+=	${LIBELF} ${LIBPROC} ${LIBRTLD_DB} ${LIBUTIL}

# Ensure that symbols aren't stripped from the test program, as they're needed
# for testing symbol lookup.
STRIP=

WARNS?=	6

.include <bsd.test.mk>
