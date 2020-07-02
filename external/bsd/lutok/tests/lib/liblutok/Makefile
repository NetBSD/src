# $NetBSD: Makefile,v 1.4 2020/07/02 13:56:10 lukem Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/liblutok

SRCDIR=		${NETBSDSRCDIR}/external/bsd/lutok/dist
.PATH:		${SRCDIR}

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/liblutok

FILESDIR=	${TESTSDIR}

TESTS_CXX=
.for test in	\
		c_gate_test \
		debug_test \
		exceptions_test \
		operations_test \
		stack_cleaner_test \
		state_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

LDADD+=		-llutok -llua
DPADD+=		${LIBLUTOK} ${LIBLUA}

.include <bsd.test.mk>
