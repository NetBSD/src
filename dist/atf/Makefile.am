#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
# CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# -------------------------------------------------------------------------
# Top directory.
# -------------------------------------------------------------------------

doc_DATA = AUTHORS COPYING NEWS README ROADMAP
EXTRA_DIST = $(doc_DATA)

dist-hook: $(srcdir)/admin/revision-dist.h check-install check-style

AM_CPPFLAGS = -DATF_ARCH=\"$(atf_arch)\" \
              -DATF_CONFDIR=\"$(atf_confdir)\" \
              -DATF_LIBEXECDIR=\"$(libexecdir)\" \
              -DATF_MACHINE=\"$(atf_machine)\" \
              -DATF_PKGDATADIR=\"$(pkgdatadir)\" \
              -DATF_SHELL=\"$(ATF_SHELL)\" \
              -DATF_WORKDIR=\"$(ATF_WORKDIR)\"

ATF_COMPILE_DEPS = $(srcdir)/atf-sh/atf.init.subr
ATF_COMPILE_DEPS += tools/atf-host-compile
ATF_COMPILE_SH = ./tools/atf-host-compile

# -------------------------------------------------------------------------
# `admin' directory.
# -------------------------------------------------------------------------

.PHONY: check-install
check-install:
	$(srcdir)/admin/check-install.sh $(srcdir)/INSTALL

.PHONY: check-style
check-style:
	$(srcdir)/admin/check-style.sh

EXTRA_DIST += admin/check-install.sh \
              admin/check-style-common.awk \
              admin/check-style-c.awk \
              admin/check-style-cpp.awk \
              admin/check-style-man.awk \
              admin/check-style-shell.awk \
              admin/check-style.sh

.PHONY: admin/revision.h
admin/revision.h:
	@if [ -n "$(MTN)" -a -d $(top_srcdir)/_MTN ]; then \
	    test -d admin || mkdir -p admin; \
	    MTN="$(MTN)" ROOT="$(top_srcdir)" \
	        $(top_srcdir)/admin/generate-revision.sh >admin/revision.h; \
	else \
	    rm -f admin/revision.h; \
	fi
CLEANFILES = admin/revision.h

$(srcdir)/admin/revision-dist.h: admin/revision.h
	@if [ -f admin/revision.h ]; then \
	    cp admin/revision.h $(srcdir)/admin/revision-dist.h; \
	    echo '#define PACKAGE_REVISION_CACHED 1' \
	        >>$(srcdir)/admin/revision-dist.h; \
	else \
	    [ -f $(srcdir)/admin/revision-dist.h ]; \
	fi
EXTRA_DIST += admin/revision-dist.h

# -------------------------------------------------------------------------
# `atf-c' directory.
# -------------------------------------------------------------------------

lib_LTLIBRARIES = libatf-c.la
libatf_c_la_SOURCES = atf-c/config.c \
                      atf-c/config.h \
                      atf-c/dynstr.c \
                      atf-c/dynstr.h \
                      atf-c/env.c \
                      atf-c/env.h \
                      atf-c/error.c \
                      atf-c/error.h \
                      atf-c/expand.c \
                      atf-c/expand.h \
                      atf-c/fs.c \
                      atf-c/fs.h \
                      atf-c/io.c \
                      atf-c/io.h \
                      atf-c/list.c \
                      atf-c/list.h \
                      atf-c/macros.h \
                      atf-c/map.c \
                      atf-c/map.h \
                      atf-c/object.c \
                      atf-c/object.h \
                      atf-c/sanity.c \
                      atf-c/sanity.h \
                      atf-c/signals.c \
                      atf-c/signals.h \
                      atf-c/text.c \
                      atf-c/text.h \
                      atf-c/ui.c \
                      atf-c/ui.h \
                      atf-c/user.c \
                      atf-c/user.h \
                      atf-c/tc.c \
                      atf-c/tc.h \
                      atf-c/tcr.c \
                      atf-c/tcr.h \
                      atf-c/tp.c \
                      atf-c/tp.h \
                      atf-c/tp_main.c
nodist_libatf_c_la_SOURCES = \
                      atf-c/defs.h

# XXX For some reason, the nodist line above does not work as expected.
# Work this problem around.
dist-hook: kill-defs-h
kill-defs-h:
	rm -f $(distdir)/atf-c/defs.h

include_HEADERS = atf-c.h
atf_c_HEADERS = atf-c/config.h \
                atf-c/defs.h \
                atf-c/dynstr.h \
                atf-c/env.h \
                atf-c/error.h \
                atf-c/expand.h \
                atf-c/fs.h \
                atf-c/io.h \
                atf-c/list.h \
                atf-c/macros.h \
                atf-c/map.h \
                atf-c/object.h \
                atf-c/sanity.h \
                atf-c/signals.h \
                atf-c/tc.h \
                atf-c/tcr.h \
                atf-c/text.h \
                atf-c/tp.h \
                atf-c/ui.h \
                atf-c/user.h
atf_cdir = $(includedir)/atf-c

dist_man_MANS = atf-c/atf-c-api.3

# -------------------------------------------------------------------------
# `atf-c++' directory.
# -------------------------------------------------------------------------

lib_LTLIBRARIES += libatf-c++.la
libatf_c___la_LDFLAGS = -L.
libatf_c___la_LIBADD = -latf-c
libatf_c___la_SOURCES = atf-c++/application.cpp \
                        atf-c++/application.hpp \
                        atf-c++/atffile.cpp \
                        atf-c++/atffile.hpp \
                        atf-c++/config.cpp \
                        atf-c++/config.hpp \
                        atf-c++/env.cpp \
                        atf-c++/env.hpp \
                        atf-c++/exceptions.cpp \
                        atf-c++/exceptions.hpp \
                        atf-c++/expand.cpp \
                        atf-c++/expand.hpp \
                        atf-c++/formats.cpp \
                        atf-c++/formats.hpp \
                        atf-c++/fs.cpp \
                        atf-c++/fs.hpp \
                        atf-c++/io.cpp \
                        atf-c++/io.hpp \
                        atf-c++/macros.hpp \
                        atf-c++/parser.cpp \
                        atf-c++/parser.hpp \
                        atf-c++/sanity.hpp \
                        atf-c++/signals.cpp \
                        atf-c++/signals.hpp \
                        atf-c++/tests.cpp \
                        atf-c++/tests.hpp \
                        atf-c++/text.cpp \
                        atf-c++/text.hpp \
                        atf-c++/ui.cpp \
                        atf-c++/ui.hpp \
                        atf-c++/user.cpp \
                        atf-c++/user.hpp \
                        atf-c++/utils.hpp

include_HEADERS += atf-c++.hpp
atf_c___HEADERS = atf-c++/application.hpp \
                  atf-c++/atffile.hpp \
                  atf-c++/config.hpp \
                  atf-c++/env.hpp \
                  atf-c++/exceptions.hpp \
                  atf-c++/expand.hpp \
                  atf-c++/formats.hpp \
                  atf-c++/fs.hpp \
                  atf-c++/io.hpp \
                  atf-c++/macros.hpp \
                  atf-c++/parser.hpp \
                  atf-c++/sanity.hpp \
                  atf-c++/tests.hpp \
                  atf-c++/text.hpp \
                  atf-c++/ui.hpp \
                  atf-c++/user.hpp \
                  atf-c++/utils.hpp
atf_c__dir = $(includedir)/atf-c++

dist_man_MANS += atf-c++/atf-c++-api.3

# -------------------------------------------------------------------------
# `atf-sh' directory.
# -------------------------------------------------------------------------

atf_sh_DATA = atf-sh/atf.footer.subr \
              atf-sh/atf.header.subr \
              atf-sh/atf.init.subr
atf_shdir = $(pkgdatadir)
EXTRA_DIST += $(atf_sh_DATA)

dist_man_MANS += atf-sh/atf-sh-api.3

# -------------------------------------------------------------------------
# `data' directory.
# -------------------------------------------------------------------------

cssdir = $(atf_cssdir)
css_DATA = data/tests-results.css
EXTRA_DIST += $(css_DATA)

dtddir = $(atf_dtddir)
dtd_DATA = data/tests-results.dtd
EXTRA_DIST += $(dtd_DATA)

egdir = $(atf_egdir)
eg_DATA = data/atf-run.hooks
EXTRA_DIST += $(eg_DATA)

pkgconfigdir = $(atf_pkgconfigdir)
pkgconfig_DATA = data/atf-c.pc data/atf-c++.pc
CLEANFILES += data/atf-c.pc data/atf-c++.pc
EXTRA_DIST += data/atf-c.pc.in data/atf-c++.pc.in
data/atf-c.pc: $(srcdir)/data/atf-c.pc.in
	test -d data || mkdir -p data
	sed -e 's,__ATF_VERSION__,@PACKAGE_VERSION@,g' \
	    -e 's,__CC__,$(CC),g' \
	    -e 's,__INCLUDEDIR__,$(includedir),g' \
	    -e 's,__LIBDIR__,$(libdir),g' \
	    <$(srcdir)/data/atf-c.pc.in >data/atf-c.pc.tmp
	mv data/atf-c.pc.tmp data/atf-c.pc
data/atf-c++.pc: $(srcdir)/data/atf-c++.pc.in
	test -d data || mkdir -p data
	sed -e 's,__ATF_VERSION__,@PACKAGE_VERSION@,g' \
	    -e 's,__CXX__,$(CXX),g' \
	    -e 's,__INCLUDEDIR__,$(includedir),g' \
	    -e 's,__LIBDIR__,$(libdir),g' \
	    <$(srcdir)/data/atf-c++.pc.in >data/atf-c++.pc.tmp
	mv data/atf-c++.pc.tmp data/atf-c++.pc

xsldir = $(atf_xsldir)
xsl_DATA = data/tests-results.xsl
EXTRA_DIST += $(xsl_DATA)

# -------------------------------------------------------------------------
# `doc' directory.
# -------------------------------------------------------------------------

man_MANS = doc/atf.7
CLEANFILES += doc/atf.7
EXTRA_DIST += doc/atf.7.in

dist_man_MANS += doc/atf-formats.5 \
                 doc/atf-test-case.4 \
                 doc/atf-test-program.1

doc/atf.7: $(srcdir)/doc/atf.7.in
	test -d doc || mkdir -p doc
	sed -e 's,__DOCDIR__,$(docdir),g' \
	    <$(srcdir)/doc/atf.7.in >doc/atf.7.tmp
	mv doc/atf.7.tmp doc/atf.7

# -------------------------------------------------------------------------
# `m4' directory.
# -------------------------------------------------------------------------

ACLOCAL_AMFLAGS = -I m4

# -------------------------------------------------------------------------
# `tests/bootstrap' directory.
# -------------------------------------------------------------------------

check_PROGRAMS = tests/bootstrap/h_app_empty
tests_bootstrap_h_app_empty_SOURCES = tests/bootstrap/h_app_empty.cpp
tests_bootstrap_h_app_empty_LDADD = -latf-c++
tests_bootstrap_h_app_empty_LDFLAGS = -L.

check_PROGRAMS += tests/bootstrap/h_app_opts_args
tests_bootstrap_h_app_opts_args_SOURCES = tests/bootstrap/h_app_opts_args.cpp
tests_bootstrap_h_app_opts_args_LDADD = -latf-c++
tests_bootstrap_h_app_opts_args_LDFLAGS = -L.

check_PROGRAMS += tests/bootstrap/h_tp_basic_c
tests_bootstrap_h_tp_basic_c_SOURCES = tests/bootstrap/h_tp_basic_c.c
tests_bootstrap_h_tp_basic_c_LDADD = -latf-c
tests_bootstrap_h_tp_basic_c_LDFLAGS = -L.

check_PROGRAMS += tests/bootstrap/h_tp_basic_cpp
tests_bootstrap_h_tp_basic_cpp_SOURCES = tests/bootstrap/h_tp_basic_cpp.cpp
tests_bootstrap_h_tp_basic_cpp_LDADD = -latf-c++
tests_bootstrap_h_tp_basic_cpp_LDFLAGS = -L.

check_SCRIPTS = tests/bootstrap/h_tp_basic_sh
CLEANFILES += tests/bootstrap/h_tp_basic_sh
EXTRA_DIST += tests/bootstrap/h_tp_basic_sh.sh
tests/bootstrap/h_tp_basic_sh: $(srcdir)/tests/bootstrap/h_tp_basic_sh.sh \
                               $(ATF_COMPILE_DEPS)
	test -d tests/bootstrap || mkdir -p tests/bootstrap
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/bootstrap/h_tp_basic_sh.sh

check_SCRIPTS += tests/bootstrap/h_tp_atf_check_sh
CLEANFILES += tests/bootstrap/h_tp_atf_check_sh
EXTRA_DIST += tests/bootstrap/h_tp_atf_check_sh.sh
tests/bootstrap/h_tp_atf_check_sh: \
		$(srcdir)/tests/bootstrap/h_tp_atf_check_sh.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/bootstrap || mkdir -p tests/bootstrap
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/bootstrap/h_tp_atf_check_sh.sh

TESTS=		$(srcdir)/tests/bootstrap/testsuite

DISTCLEANFILES = \
		tests/bootstrap/atconfig \
		testsuite.lineno \
		testsuite.log

distclean-local:
	-rm -rf testsuite.dir

EXTRA_DIST +=	tests/bootstrap/testsuite \
		tests/bootstrap/package.m4 \
		tests/bootstrap/testsuite.at \
		$(testsuite_incs)

testsuite_incs=	$(srcdir)/tests/bootstrap/t_application_help.at \
		$(srcdir)/tests/bootstrap/t_application_opts_args.at \
		$(srcdir)/tests/bootstrap/t_atf_config.at \
		$(srcdir)/tests/bootstrap/t_atf_format.at \
		$(srcdir)/tests/bootstrap/t_atf_run.at \
		$(srcdir)/tests/bootstrap/t_subr_atf_check.at \
		$(srcdir)/tests/bootstrap/t_test_program_compare.at \
		$(srcdir)/tests/bootstrap/t_test_program_filter.at \
		$(srcdir)/tests/bootstrap/t_test_program_list.at \
		$(srcdir)/tests/bootstrap/t_test_program_run.at

$(srcdir)/tests/bootstrap/package.m4: $(top_srcdir)/configure.ac
	{ \
	echo '# Signature of the current package.'; \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])'; \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])'; \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])'; \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])'; \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >$(srcdir)/tests/bootstrap/package.m4

$(srcdir)/tests/bootstrap/testsuite: $(srcdir)/tests/bootstrap/testsuite.at \
                                     $(testsuite_incs) \
                                     $(srcdir)/tests/bootstrap/package.m4
	autom4te --language=Autotest -I $(srcdir) \
	    -I $(srcdir)/tests/bootstrap \
	    $(srcdir)/tests/bootstrap/testsuite.at -o $@.tmp
	mv $@.tmp $@

# -------------------------------------------------------------------------
# `tests/atf' directory.
# -------------------------------------------------------------------------

testsdir = $(exec_prefix)/tests
pkgtestsdir = $(exec_prefix)/tests/atf

installcheck-local:
	logfile=$$(pwd)/installcheck.log; \
	cd $(pkgtestsdir); \
	$(TESTS_ENVIRONMENT) atf-run | tee $${logfile} | atf-report; \
	res=$${?}; \
	echo; \
	echo "The verbatim output of atf-run has been saved to" \
	     "installcheck.log"; \
	exit $${res}
CLEANFILES += installcheck.log

pkgtests_DATA = tests/atf/Atffile
EXTRA_DIST += $(pkgtests_DATA)

atf_atf_c_DATA = tests/atf/atf-c/Atffile
atf_atf_cdir = $(pkgtestsdir)/atf-c
EXTRA_DIST += $(atf_atf_c_DATA)

atf_atf_c_PROGRAMS = tests/atf/atf-c/t_config
tests_atf_atf_c_t_config_SOURCES = tests/atf/atf-c/t_config.c
tests_atf_atf_c_t_config_LDADD = -latf-c
tests_atf_atf_c_t_config_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_dynstr
tests_atf_atf_c_t_dynstr_SOURCES = tests/atf/atf-c/t_dynstr.c
tests_atf_atf_c_t_dynstr_LDADD = -latf-c
tests_atf_atf_c_t_dynstr_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_env
tests_atf_atf_c_t_env_SOURCES = tests/atf/atf-c/t_env.c
tests_atf_atf_c_t_env_LDADD = -latf-c
tests_atf_atf_c_t_env_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_error
tests_atf_atf_c_t_error_SOURCES = tests/atf/atf-c/t_error.c
tests_atf_atf_c_t_error_LDADD = -latf-c
tests_atf_atf_c_t_error_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_expand
tests_atf_atf_c_t_expand_SOURCES = tests/atf/atf-c/t_expand.c
tests_atf_atf_c_t_expand_LDADD = -latf-c
tests_atf_atf_c_t_expand_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_fs
tests_atf_atf_c_t_fs_SOURCES = tests/atf/atf-c/t_fs.c
tests_atf_atf_c_t_fs_LDADD = -latf-c
tests_atf_atf_c_t_fs_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_io
tests_atf_atf_c_t_io_SOURCES = tests/atf/atf-c/t_io.c
tests_atf_atf_c_t_io_LDADD = -latf-c
tests_atf_atf_c_t_io_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_list
tests_atf_atf_c_t_list_SOURCES = tests/atf/atf-c/t_list.c
tests_atf_atf_c_t_list_LDADD = -latf-c
tests_atf_atf_c_t_list_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_macros
tests_atf_atf_c_t_macros_SOURCES = tests/atf/atf-c/t_macros.c
tests_atf_atf_c_t_macros_LDADD = -latf-c
tests_atf_atf_c_t_macros_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_map
tests_atf_atf_c_t_map_SOURCES = tests/atf/atf-c/t_map.c
tests_atf_atf_c_t_map_LDADD = -latf-c
tests_atf_atf_c_t_map_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_signals
tests_atf_atf_c_t_signals_SOURCES = tests/atf/atf-c/t_signals.c
tests_atf_atf_c_t_signals_LDADD = -latf-c
tests_atf_atf_c_t_signals_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_tc
tests_atf_atf_c_t_tc_SOURCES = tests/atf/atf-c/t_tc.c
tests_atf_atf_c_t_tc_LDADD = -latf-c
tests_atf_atf_c_t_tc_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_tcr
tests_atf_atf_c_t_tcr_SOURCES = tests/atf/atf-c/t_tcr.c
tests_atf_atf_c_t_tcr_LDADD = -latf-c
tests_atf_atf_c_t_tcr_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_sanity
tests_atf_atf_c_t_sanity_SOURCES = tests/atf/atf-c/t_sanity.c
tests_atf_atf_c_t_sanity_LDADD = -latf-c
tests_atf_atf_c_t_sanity_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_text
tests_atf_atf_c_t_text_SOURCES = tests/atf/atf-c/t_text.c
tests_atf_atf_c_t_text_LDADD = -latf-c
tests_atf_atf_c_t_text_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_ui
tests_atf_atf_c_t_ui_SOURCES = tests/atf/atf-c/t_ui.c
tests_atf_atf_c_t_ui_LDADD = -latf-c
tests_atf_atf_c_t_ui_LDFLAGS = -L.

atf_atf_c_PROGRAMS += tests/atf/atf-c/t_user
tests_atf_atf_c_t_user_SOURCES = tests/atf/atf-c/t_user.c
tests_atf_atf_c_t_user_LDADD = -latf-c
tests_atf_atf_c_t_user_LDFLAGS = -L.

atf_atf_c___DATA = tests/atf/atf-c++/Atffile
atf_atf_c__dir = $(pkgtestsdir)/atf-c++
EXTRA_DIST += $(atf_atf_c___DATA)

atf_atf_c___PROGRAMS = tests/atf/atf-c++/t_config
tests_atf_atf_c___t_config_SOURCES = tests/atf/atf-c++/t_config.cpp
tests_atf_atf_c___t_config_LDADD = -latf-c++
tests_atf_atf_c___t_config_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_env
tests_atf_atf_c___t_env_SOURCES = tests/atf/atf-c++/t_env.cpp
tests_atf_atf_c___t_env_LDADD = -latf-c++
tests_atf_atf_c___t_env_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_expand
tests_atf_atf_c___t_expand_SOURCES = tests/atf/atf-c++/t_expand.cpp
tests_atf_atf_c___t_expand_LDADD = -latf-c++
tests_atf_atf_c___t_expand_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_fs
tests_atf_atf_c___t_fs_SOURCES = tests/atf/atf-c++/t_fs.cpp
tests_atf_atf_c___t_fs_LDADD = -latf-c++
tests_atf_atf_c___t_fs_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_io
tests_atf_atf_c___t_io_SOURCES = tests/atf/atf-c++/t_io.cpp
tests_atf_atf_c___t_io_LDADD = -latf-c++
tests_atf_atf_c___t_io_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_macros
tests_atf_atf_c___t_macros_SOURCES = tests/atf/atf-c++/t_macros.cpp
tests_atf_atf_c___t_macros_LDADD = -latf-c++
tests_atf_atf_c___t_macros_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_parser
tests_atf_atf_c___t_parser_SOURCES = tests/atf/atf-c++/t_parser.cpp
tests_atf_atf_c___t_parser_LDADD = -latf-c++
tests_atf_atf_c___t_parser_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_signals
tests_atf_atf_c___t_signals_SOURCES = tests/atf/atf-c++/t_signals.cpp
tests_atf_atf_c___t_signals_LDADD = -latf-c++
tests_atf_atf_c___t_signals_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_tests
tests_atf_atf_c___t_tests_SOURCES = tests/atf/atf-c++/t_tests.cpp
tests_atf_atf_c___t_tests_LDADD = -latf-c++
tests_atf_atf_c___t_tests_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_text
tests_atf_atf_c___t_text_SOURCES = tests/atf/atf-c++/t_text.cpp
tests_atf_atf_c___t_text_LDADD = -latf-c++
tests_atf_atf_c___t_text_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_user
tests_atf_atf_c___t_user_SOURCES = tests/atf/atf-c++/t_user.cpp
tests_atf_atf_c___t_user_LDADD = -latf-c++
tests_atf_atf_c___t_user_LDFLAGS = -L.

atf_atf_c___PROGRAMS += tests/atf/atf-c++/t_utils
tests_atf_atf_c___t_utils_SOURCES = tests/atf/atf-c++/t_utils.cpp
tests_atf_atf_c___t_utils_LDADD = -latf-c++
tests_atf_atf_c___t_utils_LDFLAGS = -L.

atf_atf_sh_DATA = tests/atf/atf-sh/Atffile
atf_atf_shdir = $(pkgtestsdir)/atf-sh
EXTRA_DIST += $(atf_atf_sh_DATA)

atf_atf_sh_SCRIPTS = tests/atf/atf-sh/h_misc
CLEANFILES += tests/atf/atf-sh/h_misc
EXTRA_DIST += tests/atf/atf-sh/h_misc.sh
tests/atf/atf-sh/h_misc: \
		$(srcdir)/tests/atf/atf-sh/h_misc.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/atf-sh || mkdir -p tests/atf/atf-sh
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/atf-sh/h_misc.sh

atf_atf_sh_SCRIPTS += tests/atf/atf-sh/t_atf_check
CLEANFILES += tests/atf/atf-sh/t_atf_check
EXTRA_DIST += tests/atf/atf-sh/t_atf_check.sh
tests/atf/atf-sh/t_atf_check: \
		$(srcdir)/tests/atf/atf-sh/t_atf_check.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/atf-sh || mkdir -p tests/atf/atf-sh
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/atf-sh/t_atf_check.sh

atf_atf_sh_SCRIPTS += tests/atf/atf-sh/t_config
CLEANFILES += tests/atf/atf-sh/t_config
EXTRA_DIST += tests/atf/atf-sh/t_config.sh
tests/atf/atf-sh/t_config: \
		$(srcdir)/tests/atf/atf-sh/t_config.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/atf-sh || mkdir -p tests/atf/atf-sh
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/atf-sh/t_config.sh

atf_atf_sh_SCRIPTS += tests/atf/atf-sh/t_normalize
CLEANFILES += tests/atf/atf-sh/t_normalize
EXTRA_DIST += tests/atf/atf-sh/t_normalize.sh
tests/atf/atf-sh/t_normalize: \
		$(srcdir)/tests/atf/atf-sh/t_normalize.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/atf-sh || mkdir -p tests/atf/atf-sh
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/atf-sh/t_normalize.sh

atf_atf_sh_SCRIPTS += tests/atf/atf-sh/t_tc
CLEANFILES += tests/atf/atf-sh/t_tc
EXTRA_DIST += tests/atf/atf-sh/t_tc.sh
tests/atf/atf-sh/t_tc: \
		$(srcdir)/tests/atf/atf-sh/t_tc.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/atf-sh || mkdir -p tests/atf/atf-sh
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/atf-sh/t_tc.sh

atf_atf_sh_SCRIPTS += tests/atf/atf-sh/t_tp
CLEANFILES += tests/atf/atf-sh/t_tp
EXTRA_DIST += tests/atf/atf-sh/t_tp.sh
tests/atf/atf-sh/t_tp: \
		$(srcdir)/tests/atf/atf-sh/t_tp.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/atf-sh || mkdir -p tests/atf/atf-sh
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/atf-sh/t_tp.sh

atf_data_DATA = tests/atf/data/Atffile
atf_datadir = $(pkgtestsdir)/data
EXTRA_DIST += $(atf_data_DATA)

atf_data_SCRIPTS = tests/atf/data/t_pkg_config
CLEANFILES += tests/atf/data/t_pkg_config
EXTRA_DIST += tests/atf/data/t_pkg_config.sh
tests/atf/data/t_pkg_config: \
		$(srcdir)/tests/atf/data/t_pkg_config.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/data || mkdir -p tests/atf/data
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/data/t_pkg_config.sh

atf_formats_DATA = tests/atf/formats/Atffile \
                   tests/atf/formats/d_atffile_1 \
                   tests/atf/formats/d_atffile_1.expout \
                   tests/atf/formats/d_atffile_2 \
                   tests/atf/formats/d_atffile_2.expout \
                   tests/atf/formats/d_atffile_3 \
                   tests/atf/formats/d_atffile_3.expout \
                   tests/atf/formats/d_atffile_4 \
                   tests/atf/formats/d_atffile_4.expout \
                   tests/atf/formats/d_atffile_5 \
                   tests/atf/formats/d_atffile_5.expout \
                   tests/atf/formats/d_atffile_50 \
                   tests/atf/formats/d_atffile_50.experr \
                   tests/atf/formats/d_atffile_51 \
                   tests/atf/formats/d_atffile_51.experr \
                   tests/atf/formats/d_atffile_52 \
                   tests/atf/formats/d_atffile_52.experr \
                   tests/atf/formats/d_atffile_53 \
                   tests/atf/formats/d_atffile_53.experr \
                   tests/atf/formats/d_atffile_53.expout \
                   tests/atf/formats/d_atffile_54 \
                   tests/atf/formats/d_atffile_54.experr \
                   tests/atf/formats/d_atffile_6 \
                   tests/atf/formats/d_atffile_6.expout \
                   tests/atf/formats/d_config_1 \
                   tests/atf/formats/d_config_1.expout \
                   tests/atf/formats/d_config_2 \
                   tests/atf/formats/d_config_2.expout \
                   tests/atf/formats/d_config_3 \
                   tests/atf/formats/d_config_3.expout \
                   tests/atf/formats/d_config_4 \
                   tests/atf/formats/d_config_4.expout \
                   tests/atf/formats/d_config_50 \
                   tests/atf/formats/d_config_50.experr \
                   tests/atf/formats/d_config_51 \
                   tests/atf/formats/d_config_51.experr \
                   tests/atf/formats/d_config_52 \
                   tests/atf/formats/d_config_52.experr \
                   tests/atf/formats/d_config_53 \
                   tests/atf/formats/d_config_53.experr \
                   tests/atf/formats/d_config_53.expout \
                   tests/atf/formats/d_config_54 \
                   tests/atf/formats/d_config_54.experr \
                   tests/atf/formats/d_headers_1 \
                   tests/atf/formats/d_headers_1.experr \
                   tests/atf/formats/d_headers_10 \
                   tests/atf/formats/d_headers_10.experr \
                   tests/atf/formats/d_headers_11 \
                   tests/atf/formats/d_headers_11.experr \
                   tests/atf/formats/d_headers_12 \
                   tests/atf/formats/d_headers_12.experr \
                   tests/atf/formats/d_headers_2 \
                   tests/atf/formats/d_headers_2.experr \
                   tests/atf/formats/d_headers_3 \
                   tests/atf/formats/d_headers_3.experr \
                   tests/atf/formats/d_headers_4 \
                   tests/atf/formats/d_headers_4.experr \
                   tests/atf/formats/d_headers_5 \
                   tests/atf/formats/d_headers_5.experr \
                   tests/atf/formats/d_headers_6 \
                   tests/atf/formats/d_headers_6.experr \
                   tests/atf/formats/d_headers_7 \
                   tests/atf/formats/d_headers_7.experr \
                   tests/atf/formats/d_headers_8 \
                   tests/atf/formats/d_headers_8.experr \
                   tests/atf/formats/d_headers_9 \
                   tests/atf/formats/d_headers_9.experr \
                   tests/atf/formats/d_tcs_1 \
                   tests/atf/formats/d_tcs_1.errin \
                   tests/atf/formats/d_tcs_1.expout \
                   tests/atf/formats/d_tcs_1.outin \
                   tests/atf/formats/d_tcs_2 \
                   tests/atf/formats/d_tcs_2.errin \
                   tests/atf/formats/d_tcs_2.expout \
                   tests/atf/formats/d_tcs_2.outin \
                   tests/atf/formats/d_tcs_3 \
                   tests/atf/formats/d_tcs_3.errin \
                   tests/atf/formats/d_tcs_3.expout \
                   tests/atf/formats/d_tcs_3.outin \
                   tests/atf/formats/d_tcs_4 \
                   tests/atf/formats/d_tcs_4.errin \
                   tests/atf/formats/d_tcs_4.expout \
                   tests/atf/formats/d_tcs_4.outin \
                   tests/atf/formats/d_tcs_5 \
                   tests/atf/formats/d_tcs_5.errin \
                   tests/atf/formats/d_tcs_5.expout \
                   tests/atf/formats/d_tcs_5.outin \
                   tests/atf/formats/d_tcs_50 \
                   tests/atf/formats/d_tcs_50.experr \
                   tests/atf/formats/d_tcs_51 \
                   tests/atf/formats/d_tcs_51.experr \
                   tests/atf/formats/d_tcs_52 \
                   tests/atf/formats/d_tcs_52.experr \
                   tests/atf/formats/d_tcs_53 \
                   tests/atf/formats/d_tcs_53.experr \
                   tests/atf/formats/d_tcs_53.expout \
                   tests/atf/formats/d_tcs_54 \
                   tests/atf/formats/d_tcs_54.experr \
                   tests/atf/formats/d_tcs_54.expout \
                   tests/atf/formats/d_tcs_55 \
                   tests/atf/formats/d_tcs_55.experr \
                   tests/atf/formats/d_tcs_55.expout \
                   tests/atf/formats/d_tcs_56 \
                   tests/atf/formats/d_tcs_56.errin \
                   tests/atf/formats/d_tcs_56.experr \
                   tests/atf/formats/d_tcs_56.expout \
                   tests/atf/formats/d_tcs_56.outin \
                   tests/atf/formats/d_tcs_57 \
                   tests/atf/formats/d_tcs_57.errin \
                   tests/atf/formats/d_tcs_57.experr \
                   tests/atf/formats/d_tcs_57.expout \
                   tests/atf/formats/d_tcs_57.outin \
                   tests/atf/formats/d_tps_1 \
                   tests/atf/formats/d_tps_1.expout \
                   tests/atf/formats/d_tps_2 \
                   tests/atf/formats/d_tps_2.expout \
                   tests/atf/formats/d_tps_3 \
                   tests/atf/formats/d_tps_3.expout \
                   tests/atf/formats/d_tps_4 \
                   tests/atf/formats/d_tps_4.expout \
                   tests/atf/formats/d_tps_5 \
                   tests/atf/formats/d_tps_5.expout \
                   tests/atf/formats/d_tps_50 \
                   tests/atf/formats/d_tps_50.experr \
                   tests/atf/formats/d_tps_51 \
                   tests/atf/formats/d_tps_51.experr \
                   tests/atf/formats/d_tps_52 \
                   tests/atf/formats/d_tps_52.experr \
                   tests/atf/formats/d_tps_53 \
                   tests/atf/formats/d_tps_53.experr \
                   tests/atf/formats/d_tps_53.expout \
                   tests/atf/formats/d_tps_54 \
                   tests/atf/formats/d_tps_54.experr \
                   tests/atf/formats/d_tps_54.expout \
                   tests/atf/formats/d_tps_55 \
                   tests/atf/formats/d_tps_55.experr \
                   tests/atf/formats/d_tps_55.expout \
                   tests/atf/formats/d_tps_56 \
                   tests/atf/formats/d_tps_56.experr \
                   tests/atf/formats/d_tps_56.expout \
                   tests/atf/formats/d_tps_57 \
                   tests/atf/formats/d_tps_57.experr \
                   tests/atf/formats/d_tps_57.expout \
                   tests/atf/formats/d_tps_58 \
                   tests/atf/formats/d_tps_58.experr \
                   tests/atf/formats/d_tps_58.expout \
                   tests/atf/formats/d_tps_59 \
                   tests/atf/formats/d_tps_59.experr \
                   tests/atf/formats/d_tps_60 \
                   tests/atf/formats/d_tps_60.experr \
                   tests/atf/formats/d_tps_61 \
                   tests/atf/formats/d_tps_61.experr \
                   tests/atf/formats/d_tps_62 \
                   tests/atf/formats/d_tps_62.experr \
                   tests/atf/formats/d_tps_62.expout \
                   tests/atf/formats/d_tps_63 \
                   tests/atf/formats/d_tps_63.experr \
                   tests/atf/formats/d_tps_63.expout \
                   tests/atf/formats/d_tps_64 \
                   tests/atf/formats/d_tps_64.experr \
                   tests/atf/formats/d_tps_64.expout \
                   tests/atf/formats/d_tps_65 \
                   tests/atf/formats/d_tps_65.experr \
                   tests/atf/formats/d_tps_65.expout \
                   tests/atf/formats/d_tps_66 \
                   tests/atf/formats/d_tps_66.experr \
                   tests/atf/formats/d_tps_66.expout
atf_formatsdir = $(pkgtestsdir)/formats
EXTRA_DIST += $(atf_formats_DATA)

atf_formats_PROGRAMS = tests/atf/formats/h_parser
tests_atf_formats_h_parser_SOURCES = tests/atf/formats/h_parser.cpp
tests_atf_formats_h_parser_LDADD = -latf-c++
tests_atf_formats_h_parser_LDFLAGS = -L.

atf_formats_PROGRAMS += tests/atf/formats/t_writers
tests_atf_formats_t_writers_SOURCES = tests/atf/formats/t_writers.cpp
tests_atf_formats_t_writers_LDADD = -latf-c++
tests_atf_formats_t_writers_LDFLAGS = -L.

atf_formats_SCRIPTS = tests/atf/formats/t_parsers
CLEANFILES += tests/atf/formats/t_parsers
EXTRA_DIST += tests/atf/formats/t_parsers.sh
tests/atf/formats/t_parsers: \
		$(srcdir)/tests/atf/formats/t_parsers.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/formats || mkdir -p tests/atf/formats
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/formats/t_parsers.sh

atf_test_programs_DATA = tests/atf/test_programs/Atffile
atf_test_programsdir = $(pkgtestsdir)/test_programs
EXTRA_DIST += $(atf_test_programs_DATA)

EXTRA_DIST += tests/atf/test_programs/common.sh

atf_test_programs_SCRIPTS = tests/atf/test_programs/h_sh
CLEANFILES += tests/atf/test_programs/h_sh
EXTRA_DIST += tests/atf/test_programs/h_sh.sh
tests/atf/test_programs/h_sh: \
		$(srcdir)/tests/atf/test_programs/h_sh.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/h_sh.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_cleanup
CLEANFILES += tests/atf/test_programs/t_cleanup
EXTRA_DIST += tests/atf/test_programs/t_cleanup.sh
tests/atf/test_programs/t_cleanup: \
		$(srcdir)/tests/atf/test_programs/common.sh \
		$(srcdir)/tests/atf/test_programs/t_cleanup.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/common.sh \
	    $(srcdir)/tests/atf/test_programs/t_cleanup.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_config
CLEANFILES += tests/atf/test_programs/t_config
EXTRA_DIST += tests/atf/test_programs/t_config.sh
tests/atf/test_programs/t_config: \
		$(srcdir)/tests/atf/test_programs/common.sh \
		$(srcdir)/tests/atf/test_programs/t_config.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/common.sh \
	    $(srcdir)/tests/atf/test_programs/t_config.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_env
CLEANFILES += tests/atf/test_programs/t_env
EXTRA_DIST += tests/atf/test_programs/t_env.sh
tests/atf/test_programs/t_env: \
		$(srcdir)/tests/atf/test_programs/common.sh \
		$(srcdir)/tests/atf/test_programs/t_env.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/common.sh \
	    $(srcdir)/tests/atf/test_programs/t_env.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_fork
CLEANFILES += tests/atf/test_programs/t_fork
EXTRA_DIST += tests/atf/test_programs/t_fork.sh
tests/atf/test_programs/t_fork: \
		$(srcdir)/tests/atf/test_programs/common.sh \
		$(srcdir)/tests/atf/test_programs/t_fork.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/common.sh \
	    $(srcdir)/tests/atf/test_programs/t_fork.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_meta_data
CLEANFILES += tests/atf/test_programs/t_meta_data
EXTRA_DIST += tests/atf/test_programs/t_meta_data.sh
tests/atf/test_programs/t_meta_data: \
		$(srcdir)/tests/atf/test_programs/common.sh \
		$(srcdir)/tests/atf/test_programs/t_meta_data.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/common.sh \
	    $(srcdir)/tests/atf/test_programs/t_meta_data.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_srcdir
CLEANFILES += tests/atf/test_programs/t_srcdir
EXTRA_DIST += tests/atf/test_programs/t_srcdir.sh
tests/atf/test_programs/t_srcdir: \
		$(srcdir)/tests/atf/test_programs/common.sh \
		$(srcdir)/tests/atf/test_programs/t_srcdir.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/common.sh \
	    $(srcdir)/tests/atf/test_programs/t_srcdir.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_status
CLEANFILES += tests/atf/test_programs/t_status
EXTRA_DIST += tests/atf/test_programs/t_status.sh
tests/atf/test_programs/t_status: \
		$(srcdir)/tests/atf/test_programs/common.sh \
		$(srcdir)/tests/atf/test_programs/t_status.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/common.sh \
	    $(srcdir)/tests/atf/test_programs/t_status.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_workdir
CLEANFILES += tests/atf/test_programs/t_workdir
EXTRA_DIST += tests/atf/test_programs/t_workdir.sh
tests/atf/test_programs/t_workdir: \
		$(srcdir)/tests/atf/test_programs/common.sh \
		$(srcdir)/tests/atf/test_programs/t_workdir.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/common.sh \
	    $(srcdir)/tests/atf/test_programs/t_workdir.sh

atf_test_programs_PROGRAMS = tests/atf/test_programs/h_c
tests_atf_test_programs_h_c_SOURCES = tests/atf/test_programs/h_c.c
tests_atf_test_programs_h_c_LDADD = -latf-c
tests_atf_test_programs_h_c_LDFLAGS = -L.

atf_test_programs_PROGRAMS += tests/atf/test_programs/h_cpp
tests_atf_test_programs_h_cpp_SOURCES = tests/atf/test_programs/h_cpp.cpp
tests_atf_test_programs_h_cpp_LDADD = -latf-c++
tests_atf_test_programs_h_cpp_LDFLAGS = -L.

atf_tools_DATA = tests/atf/tools/Atffile
atf_toolsdir = $(pkgtestsdir)/tools
EXTRA_DIST += $(atf_tools_DATA)

atf_tools_PROGRAMS = tests/atf/tools/h_fail
tests_atf_tools_h_fail_SOURCES = tests/atf/tools/h_fail.cpp
tests_atf_tools_h_fail_LDADD = -latf-c++
tests_atf_tools_h_fail_LDFLAGS = -L.

atf_tools_PROGRAMS += tests/atf/tools/h_misc
tests_atf_tools_h_misc_SOURCES = tests/atf/tools/h_misc.cpp
tests_atf_tools_h_misc_LDADD = -latf-c++
tests_atf_tools_h_misc_LDFLAGS = -L.

atf_tools_PROGRAMS += tests/atf/tools/h_mode
tests_atf_tools_h_mode_SOURCES = tests/atf/tools/h_mode.cpp
tests_atf_tools_h_mode_LDADD = -latf-c++
tests_atf_tools_h_mode_LDFLAGS = -L.

atf_tools_PROGRAMS += tests/atf/tools/h_pass
tests_atf_tools_h_pass_SOURCES = tests/atf/tools/h_pass.cpp
tests_atf_tools_h_pass_LDADD = -latf-c++
tests_atf_tools_h_pass_LDFLAGS = -L.

atf_tools_SCRIPTS = tests/atf/tools/t_atf_cleanup
CLEANFILES += tests/atf/tools/t_atf_cleanup
EXTRA_DIST += tests/atf/tools/t_atf_cleanup.sh
tests/atf/tools/t_atf_cleanup: \
			$(srcdir)/tests/atf/tools/t_atf_cleanup.sh \
                        $(ATF_COMPILE_DEPS)
	test -d tests/atf/tools || mkdir -p tests/atf/tools
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/tools/t_atf_cleanup.sh

atf_tools_SCRIPTS += tests/atf/tools/t_atf_compile
CLEANFILES += tests/atf/tools/t_atf_compile
EXTRA_DIST += tests/atf/tools/t_atf_compile.sh
tests/atf/tools/t_atf_compile: $(srcdir)/tests/atf/tools/t_atf_compile.sh \
                               $(ATF_COMPILE_DEPS)
	test -d tests/atf/tools || mkdir -p tests/atf/tools
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/tools/t_atf_compile.sh

atf_tools_SCRIPTS += tests/atf/tools/t_atf_config
CLEANFILES += tests/atf/tools/t_atf_config
EXTRA_DIST += tests/atf/tools/t_atf_config.sh
tests/atf/tools/t_atf_config: $(srcdir)/tests/atf/tools/t_atf_config.sh \
                               $(ATF_COMPILE_DEPS)
	test -d tests/atf/tools || mkdir -p tests/atf/tools
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/tools/t_atf_config.sh

atf_tools_SCRIPTS += tests/atf/tools/t_atf_exec
CLEANFILES += tests/atf/tools/t_atf_exec
EXTRA_DIST += tests/atf/tools/t_atf_exec.sh
tests/atf/tools/t_atf_exec: $(srcdir)/tests/atf/tools/t_atf_exec.sh \
                            $(ATF_COMPILE_DEPS)
	test -d tests/atf/tools || mkdir -p tests/atf/tools
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/tools/t_atf_exec.sh

atf_tools_SCRIPTS += tests/atf/tools/t_atf_killpg
CLEANFILES += tests/atf/tools/t_atf_killpg
EXTRA_DIST += tests/atf/tools/t_atf_killpg.sh
tests/atf/tools/t_atf_killpg: $(srcdir)/tests/atf/tools/t_atf_killpg.sh \
                              $(ATF_COMPILE_DEPS)
	test -d tests/atf/tools || mkdir -p tests/atf/tools
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/tools/t_atf_killpg.sh

atf_tools_SCRIPTS += tests/atf/tools/t_atf_report
CLEANFILES += tests/atf/tools/t_atf_report
EXTRA_DIST += tests/atf/tools/t_atf_report.sh
tests/atf/tools/t_atf_report: $(srcdir)/tests/atf/tools/t_atf_report.sh \
                           $(ATF_COMPILE_DEPS)
	test -d tests/atf/tools || mkdir -p tests/atf/tools
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/tools/t_atf_report.sh

atf_tools_SCRIPTS += tests/atf/tools/t_atf_run
CLEANFILES += tests/atf/tools/t_atf_run
EXTRA_DIST += tests/atf/tools/t_atf_run.sh
tests/atf/tools/t_atf_run: $(srcdir)/tests/atf/tools/t_atf_run.sh \
                           $(ATF_COMPILE_DEPS)
	test -d tests/atf/tools || mkdir -p tests/atf/tools
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/tools/t_atf_run.sh

# -------------------------------------------------------------------------
# `tests/build' directory.
# -------------------------------------------------------------------------

check_LIBRARIES = tests/build/libuseless.a

tests_build_libuseless_a_SOURCES = \
    tests/build/t_include_atf_c_h.cpp \
    tests/build/t_include_atf_c++_hpp.cpp \
    tests/build/atf-c/t_include_config_h.c \
    tests/build/atf-c/t_include_dynstr_h.c \
    tests/build/atf-c/t_include_env_h.c \
    tests/build/atf-c/t_include_error_h.c \
    tests/build/atf-c/t_include_expand_h.c \
    tests/build/atf-c/t_include_fs_h.c \
    tests/build/atf-c/t_include_io_h.c \
    tests/build/atf-c/t_include_list_h.c \
    tests/build/atf-c/t_include_macros_h.c \
    tests/build/atf-c/t_include_map_h.c \
    tests/build/atf-c/t_include_object_h.c \
    tests/build/atf-c/t_include_sanity_h.c \
    tests/build/atf-c/t_include_signals_h.c \
    tests/build/atf-c/t_include_tc_h.c \
    tests/build/atf-c/t_include_tcr_h.c \
    tests/build/atf-c/t_include_text_h.c \
    tests/build/atf-c/t_include_tp_h.c \
    tests/build/atf-c/t_include_ui_h.c \
    tests/build/atf-c/t_use_macros_h.c \
    tests/build/atf-c++/t_include_application_hpp.cpp \
    tests/build/atf-c++/t_include_atffile_hpp.cpp \
    tests/build/atf-c++/t_include_config_hpp.cpp \
    tests/build/atf-c++/t_include_env_hpp.cpp \
    tests/build/atf-c++/t_include_exceptions_hpp.cpp \
    tests/build/atf-c++/t_include_expand_hpp.cpp \
    tests/build/atf-c++/t_include_formats_hpp.cpp \
    tests/build/atf-c++/t_include_fs_hpp.cpp \
    tests/build/atf-c++/t_include_io_hpp.cpp \
    tests/build/atf-c++/t_include_macros_hpp.cpp \
    tests/build/atf-c++/t_include_parser_hpp.cpp \
    tests/build/atf-c++/t_include_sanity_hpp.cpp \
    tests/build/atf-c++/t_include_signals_hpp.cpp \
    tests/build/atf-c++/t_include_tests_hpp.cpp \
    tests/build/atf-c++/t_include_text_hpp.cpp \
    tests/build/atf-c++/t_include_ui_hpp.cpp \
    tests/build/atf-c++/t_include_user_hpp.cpp \
    tests/build/atf-c++/t_include_utils_hpp.cpp \
    tests/build/atf-c++/t_use_macros_hpp.cpp

# -------------------------------------------------------------------------
# `tools' directory.
# -------------------------------------------------------------------------

bin_PROGRAMS = tools/atf-config
tools_atf_config_SOURCES = tools/atf-config.cpp
tools_atf_config_LDADD = -latf-c++
tools_atf_config_LDFLAGS = -L.
dist_man_MANS += tools/atf-config.1

tools/atf-host-compile: $(srcdir)/tools/atf-host-compile.sh
	sed -e 's,__ATF_PKGDATADIR__,$(srcdir)/atf-sh,g' \
	    -e 's,__ATF_SHELL__,$(ATF_SHELL),g' \
	    <$(srcdir)/tools/atf-host-compile.sh \
	    >tools/atf-host-compile.tmp
	chmod +x tools/atf-host-compile.tmp
	mv tools/atf-host-compile.tmp tools/atf-host-compile
CLEANFILES += tools/atf-host-compile
CLEANFILES += tools/atf-host-compile.tmp
EXTRA_DIST += tools/atf-host-compile.sh

libexec_PROGRAMS = tools/atf-cleanup
tools_atf_cleanup_SOURCES = tools/atf-cleanup.cpp
tools_atf_cleanup_LDADD = -latf-c++
tools_atf_cleanup_LDFLAGS = -L.
dist_man_MANS += tools/atf-cleanup.1

bin_PROGRAMS += tools/atf-compile
tools_atf_compile_SOURCES = tools/atf-compile.cpp
tools_atf_compile_LDADD = -latf-c++
tools_atf_compile_LDFLAGS = -L.
dist_man_MANS += tools/atf-compile.1

libexec_PROGRAMS += tools/atf-exec
tools_atf_exec_SOURCES = tools/atf-exec.cpp
tools_atf_exec_LDADD = -latf-c++
tools_atf_exec_LDFLAGS = -L.
dist_man_MANS += tools/atf-exec.1

libexec_PROGRAMS += tools/atf-format
tools_atf_format_SOURCES = tools/atf-format.cpp
tools_atf_format_LDADD = -latf-c++
tools_atf_format_LDFLAGS = -L.
dist_man_MANS += tools/atf-format.1

libexec_PROGRAMS += tools/atf-killpg
tools_atf_killpg_SOURCES = tools/atf-killpg.cpp
tools_atf_killpg_LDADD = -latf-c++
tools_atf_killpg_LDFLAGS = -L.
dist_man_MANS += tools/atf-killpg.1

bin_PROGRAMS += tools/atf-report
tools_atf_report_SOURCES = tools/atf-report.cpp
tools_atf_report_LDADD = -latf-c++
tools_atf_report_LDFLAGS = -L.
dist_man_MANS += tools/atf-report.1

bin_PROGRAMS += tools/atf-run
tools_atf_run_SOURCES = tools/atf-run.cpp
tools_atf_run_LDADD = -latf-c++
tools_atf_run_LDFLAGS = -L.
dist_man_MANS += tools/atf-run.1

bin_PROGRAMS += tools/atf-version
tools_atf_version_SOURCES = tools/atf-version.cpp revision.h
tools_atf_version_LDADD = -latf-c++
tools_atf_version_LDFLAGS = -L.
dist_man_MANS += tools/atf-version.1

BUILT_SOURCES = revision.h
revision.h: admin/revision.h $(srcdir)/admin/revision-dist.h
	@if [ -f admin/revision.h ]; then \
	    cmp -s admin/revision.h revision.h || \
	        cp admin/revision.h revision.h; \
	else \
	    cmp -s $(srcdir)/admin/revision-dist.h revision.h || \
	        cp $(srcdir)/admin/revision-dist.h revision.h; \
	fi
CLEANFILES += revision.h

hooksdir = $(pkgdatadir)
hooks_DATA = tools/atf-run.hooks
EXTRA_DIST += $(hooks_DATA)

# vim: syntax=make:noexpandtab:shiftwidth=8:softtabstop=8
