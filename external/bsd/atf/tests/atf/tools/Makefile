# $NetBSD: Makefile,v 1.4 2014/02/11 16:11:29 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/tools

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR:H}/lib/libatf-c  # For bconfig.h.

USE_ATF_LIBTOOLS=	yes

PROGS=
.for prog in	bad_metadata_helper \
		process_helpers
PROGS+=		${prog}
SRCS.${prog}=	${prog}.c
MAN.${prog}=	# defined
BINDIR.${prog}=	${TESTSDIR}
.endfor

TESTS_C=	expect_helpers \
		several_tcs_helper \
		zero_tcs_helper

TESTS_CXX=	application_test \
		atffile_test \
		auto_array_test \
		config_file_test \
		config_test \
		fail_helper \
		env_test \
		expand_test \
		fs_test \
		io_test \
		misc_helpers \
		parser_test \
		pass_helper \
		process_test \
		reader_test \
		requirements_test \
		signals_test \
		test_program_test \
		text_test \
		ui_test \
		user_test

TESTS_SH=	atf-config_test \
		atf-report_test \
		atf-run_test \

.include "../../../Makefile.inc"
.include <bsd.test.mk>
