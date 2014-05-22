# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:33 yamt Exp $

KYUA_LIBS=	store engine utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/store

.PATH:		${SRCDIR}/store

TESTS_CXX=	backend_test \
		dbtypes_test \
		exceptions_test \
		metadata_test \
		schema_inttest \
		transaction_test

FILESDIR=	${TESTSDIR}
FILESMODE=	444
FILES=		schema_v1.sql \
		testdata_v1.sql \
		testdata_v2.sql

.include <bsd.test.mk>
