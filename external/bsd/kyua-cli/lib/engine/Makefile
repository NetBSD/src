# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:20 tls Exp $

KYUA_LIBS=	store utils

.include <bsd.init.mk>

.PATH:		${SRCDIR}/engine
.PATH:		${SRCDIR}/engine/drivers

LIB=		engine

WARNS?=		3

SRCS=		action.cpp \
		config.cpp \
		context.cpp \
		exceptions.cpp \
		filters.cpp \
		kyuafile.cpp \
		metadata.cpp \
		test_case.cpp \
		test_program.cpp \
		test_result.cpp \
		testers.cpp

# engine/drivers subdirectory.
SRCS+=		debug_test.cpp \
		list_tests.cpp \
		run_tests.cpp \
		scan_action.cpp

.include <bsd.lib.mk>
