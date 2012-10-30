# $NetBSD: Makefile,v 1.2.2.1 2012/10/30 18:49:16 yamt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-c/detail
ATFFILE=	yes

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-c/detail

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/bsd/atf/lib/libatf-c

PROGS=			process_helpers
SRCS.process_helpers=	process_helpers.c
MAN.process_helpers=	# defined
BINDIR.process_helpers=	${TESTSDIR}

TESTS_C=
.for test in	dynstr_test \
		env_test \
		fs_test \
		list_test \
		map_test \
		process_test \
		sanity_test \
		test_helpers_test \
		text_test \
		user_test
TESTS_C+=	${test}
SRCS.${test}=	${test}.c test_helpers.c
.endfor
.undef test

# Don't warn about functions which cannot be stack smash protected as
# there are a lot of them.
COPTS.dynstr_test.c=	-Wno-stack-protector

.include <bsd.test.mk>
