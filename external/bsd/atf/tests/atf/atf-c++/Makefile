# $NetBSD: Makefile,v 1.2 2009/12/22 13:38:10 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-c++

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/atf-c++

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libatf-c

FILESDIR=	${TESTSDIR}
FILES=		d_include_application_hpp.cpp \
		d_include_atf_c++_hpp.cpp \
		d_include_atffile_hpp.cpp \
		d_include_build_hpp.cpp \
		d_include_check_hpp.cpp \
		d_include_config_hpp.cpp \
		d_include_env_hpp.cpp \
		d_include_exceptions_hpp.cpp \
		d_include_expand_hpp.cpp \
		d_include_formats_hpp.cpp \
		d_include_fs_hpp.cpp \
		d_include_io_hpp.cpp \
		d_include_macros_hpp.cpp \
		d_include_parser_hpp.cpp \
		d_include_process_hpp.cpp \
		d_include_sanity_hpp.cpp \
		d_include_signals_hpp.cpp \
		d_include_tests_hpp.cpp \
		d_include_text_hpp.cpp \
		d_include_ui_hpp.cpp \
		d_include_user_hpp.cpp \
		d_include_utils_hpp.cpp \
		d_use_macros_hpp.cpp

TESTS_CXX=
.for test in	t_application \
		t_atf_c++ \
		t_atffile \
		t_build \
		t_check \
		t_config \
		t_env \
		t_exceptions \
		t_expand \
		t_formats \
		t_fs \
		t_io \
		t_macros \
		t_parser \
		t_process \
		t_sanity \
		t_signals \
		t_tests \
		t_text \
		t_ui \
		t_user \
		t_utils
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp h_lib.cpp
.endfor
.undef test

.include <bsd.test.mk>
