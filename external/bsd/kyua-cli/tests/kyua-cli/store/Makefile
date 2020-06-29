# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:59 lukem Exp $

KYUA_LIBS=	store engine utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/store

.PATH:		${SRCDIR}/store

TESTS_CXX=
.for test in	\
		backend_test \
		dbtypes_test \
		exceptions_test \
		metadata_test \
		schema_inttest \
		transaction_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

FILESDIR=	${TESTSDIR}
FILESMODE=	444
FILES=		schema_v1.sql \
		testdata_v1.sql \
		testdata_v2.sql

.include <bsd.test.mk>
