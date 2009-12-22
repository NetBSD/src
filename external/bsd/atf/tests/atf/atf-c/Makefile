# $NetBSD: Makefile,v 1.2 2009/12/22 13:38:10 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-c

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/atf-c

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/bsd/atf/lib/libatf-c

PROGS=			h_processes
SRCS.h_processes=	h_processes.c
MAN.h_processes=	# defined
BINDIR.h_processes=	${TESTSDIR}

FILESDIR=	${TESTSDIR}
FILES=		d_include_atf_c_h.c \
		d_include_build_h.c \
		d_include_check_h.c \
		d_include_config_h.c \
		d_include_dynstr_h.c \
		d_include_env_h.c \
		d_include_error_fwd_h.c \
		d_include_error_h.c \
		d_include_expand_h.c \
		d_include_fs_h.c \
		d_include_io_h.c \
		d_include_list_h.c \
		d_include_macros_h.c \
		d_include_map_h.c \
		d_include_object_h.c \
		d_include_process_h.c \
		d_include_sanity_h.c \
		d_include_signals_h.c \
		d_include_tc_h.c \
		d_include_tcr_h.c \
		d_include_text_h.c \
		d_include_tp_h.c \
		d_include_ui_h.c \
		d_include_user_h.c \
		d_use_macros_h.c

TESTS_C=
.for test in	t_atf_c \
		t_build \
		t_check \
		t_config \
		t_dynstr \
		t_env \
		t_error \
		t_expand \
		t_fs \
		t_h_lib \
		t_io \
		t_list \
		t_macros \
		t_map \
		t_process \
		t_sanity \
		t_signals \
		t_tc \
		t_tcr \
		t_text \
		t_ui \
		t_user
TESTS_C+=	${test}
SRCS.${test}=	${test}.c h_lib.c
.endfor
.undef test

.include <bsd.test.mk>
