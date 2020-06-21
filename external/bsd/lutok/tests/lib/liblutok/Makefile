# $NetBSD: Makefile,v 1.3 2020/06/21 14:12:50 lukem Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/liblutok

SRCDIR=		${NETBSDSRCDIR}/external/bsd/lutok/dist
.PATH:		${SRCDIR}

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/liblutok

# lutok uses auto_ptr; g++ 8 complains about it
CXXFLAGS+=	-Wno-deprecated-declarations

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
