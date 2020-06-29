# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:58 lukem Exp $

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/bootstrap
ATFFILE=	no

.PATH:		${SRCDIR}/bootstrap

FILES=			testsuite
FILESDIR=		${TESTSDIR}
FILESMODE=		${BINMODE}

PROGS_CXX=		atf_helpers
SRCS.atf_helpers=	atf_helpers.cpp
BINDIR.atf_helpers=	${TESTSDIR}
MAN.atf_helpers=	# none
LDADD.atf_helpers=	-latf-c++ -latf-c
DPADD.atf_helpers=	${LIBATF_CXX} ${LIBATF_C}

PROGS_CXX+=		plain_helpers
SRCS.plain_helpers=	plain_helpers.cpp
BINDIR.plain_helpers=	${TESTSDIR}
MAN.plain_helpers=	# none

.include <bsd.files.mk>
.include <bsd.test.mk>
