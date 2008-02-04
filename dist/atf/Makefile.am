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
# 3. All advertising materials mentioning features or use of this
#    software must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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

ATF_COMPILE_DEPS = $(srcdir)/subrs/atf.init.subr
ATF_COMPILE_DEPS += tools/atf-host-compile
ATF_COMPILE_SH = ./tools/atf-host-compile

# -------------------------------------------------------------------------
# `admin' directory.
# -------------------------------------------------------------------------

.PHONY: check-install
check-install:
	$(srcdir)/admin/check-install.sh $(srcdir)/INSTALL

.phony: check-style
check-style:
	$(srcdir)/admin/check-style.sh

EXTRA_DIST += admin/check-install.sh \
              admin/check-style-common.awk \
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
# `atf' directory.
# -------------------------------------------------------------------------

lib_LTLIBRARIES = libatf.la
libatf_la_SOURCES = atf/application.cpp \
                    atf/application.hpp \
                    atf/atffile.cpp \
                    atf/atffile.hpp \
                    atf/config.cpp \
                    atf/config.hpp \
                    atf/env.cpp \
                    atf/env.hpp \
                    atf/exceptions.cpp \
                    atf/exceptions.hpp \
                    atf/expand.cpp \
                    atf/expand.hpp \
                    atf/formats.cpp \
                    atf/formats.hpp \
                    atf/fs.cpp \
                    atf/fs.hpp \
                    atf/io.cpp \
                    atf/io.hpp \
                    atf/macros.hpp \
                    atf/parser.cpp \
                    atf/parser.hpp \
                    atf/sanity.cpp \
                    atf/sanity.hpp \
                    atf/signals.cpp \
                    atf/signals.hpp \
                    atf/tests.cpp \
                    atf/tests.hpp \
                    atf/text.cpp \
                    atf/text.hpp \
                    atf/ui.cpp \
                    atf/ui.hpp \
                    atf/user.cpp \
                    atf/user.hpp \
                    atf/utils.hpp

include_HEADERS = atf.hpp
atf_HEADERS = atf/application.hpp \
              atf/atffile.hpp \
              atf/config.hpp \
              atf/env.hpp \
              atf/exceptions.hpp \
              atf/expand.hpp \
              atf/formats.hpp \
              atf/fs.hpp \
              atf/io.hpp \
              atf/macros.hpp \
              atf/parser.hpp \
              atf/sanity.hpp \
              atf/tests.hpp \
              atf/text.hpp \
              atf/ui.hpp \
              atf/user.hpp \
              atf/utils.hpp
atfdir = $(includedir)/atf

dist_man_MANS = atf/atf-c++-api.3

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
pkgconfig_DATA = data/atf.pc
CLEANFILES += data/atf.pc
EXTRA_DIST += data/atf.pc.in
data/atf.pc: $(srcdir)/data/atf.pc.in
	test -d data || mkdir -p data
	sed -e 's,__ATF_VERSION__,@PACKAGE_VERSION@,g' \
	    -e 's,__CXX__,$(CXX),g' \
	    -e 's,__INCLUDEDIR__,$(includedir),g' \
	    -e 's,__LIBDIR__,$(libdir),g' \
	    <$(srcdir)/data/atf.pc.in >data/atf.pc.tmp
	mv data/atf.pc.tmp data/atf.pc

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
# `subrs' directory.
# -------------------------------------------------------------------------

subrs_DATA = subrs/atf.footer.subr \
             subrs/atf.header.subr \
             subrs/atf.init.subr
subrsdir = $(pkgdatadir)
EXTRA_DIST += $(subrs_DATA)

dist_man_MANS += subrs/atf-sh-api.3

# -------------------------------------------------------------------------
# `tests/bootstrap' directory.
# -------------------------------------------------------------------------

check_PROGRAMS = tests/bootstrap/h_app_empty
tests_bootstrap_h_app_empty_SOURCES = tests/bootstrap/h_app_empty.cpp
tests_bootstrap_h_app_empty_LDADD = -latf
tests_bootstrap_h_app_empty_LDFLAGS = -L.

check_PROGRAMS += tests/bootstrap/h_app_opts_args
tests_bootstrap_h_app_opts_args_SOURCES = tests/bootstrap/h_app_opts_args.cpp
tests_bootstrap_h_app_opts_args_LDADD = -latf
tests_bootstrap_h_app_opts_args_LDFLAGS = -L.

check_PROGRAMS += tests/bootstrap/h_tp_basic_cpp
tests_bootstrap_h_tp_basic_cpp_SOURCES = tests/bootstrap/h_tp_basic_cpp.cpp
tests_bootstrap_h_tp_basic_cpp_LDADD = -latf
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
tests_atf_formats_h_parser_LDADD = -latf
tests_atf_formats_h_parser_LDFLAGS = -L.

atf_formats_PROGRAMS += tests/atf/formats/t_writers
tests_atf_formats_t_writers_SOURCES = tests/atf/formats/t_writers.cpp
tests_atf_formats_t_writers_LDADD = -latf
tests_atf_formats_t_writers_LDFLAGS = -L.

atf_formats_SCRIPTS = tests/atf/formats/t_parsers
CLEANFILES += tests/atf/formats/t_parsers
EXTRA_DIST += tests/atf/formats/t_parsers.sh
tests/atf/formats/t_parsers: \
		$(srcdir)/tests/atf/formats/t_parsers.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/formats || mkdir -p tests/atf/formats
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/formats/t_parsers.sh

atf_sh_interface_DATA = tests/atf/sh_interface/Atffile
atf_sh_interfacedir = $(pkgtestsdir)/sh_interface
EXTRA_DIST += $(atf_sh_interface_DATA)

atf_sh_interface_SCRIPTS = tests/atf/sh_interface/h_misc
CLEANFILES += tests/atf/sh_interface/h_misc
EXTRA_DIST += tests/atf/sh_interface/h_misc.sh
tests/atf/sh_interface/h_misc: \
		$(srcdir)/tests/atf/sh_interface/h_misc.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/sh_interface || mkdir -p tests/atf/sh_interface
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/sh_interface/h_misc.sh

atf_sh_interface_SCRIPTS += tests/atf/sh_interface/t_atf_check
CLEANFILES += tests/atf/sh_interface/t_atf_check
EXTRA_DIST += tests/atf/sh_interface/t_atf_check.sh
tests/atf/sh_interface/t_atf_check: \
		$(srcdir)/tests/atf/sh_interface/t_atf_check.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/sh_interface || mkdir -p tests/atf/sh_interface
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/sh_interface/t_atf_check.sh

atf_sh_interface_SCRIPTS += tests/atf/sh_interface/t_config
CLEANFILES += tests/atf/sh_interface/t_config
EXTRA_DIST += tests/atf/sh_interface/t_config.sh
tests/atf/sh_interface/t_config: \
		$(srcdir)/tests/atf/sh_interface/t_config.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/sh_interface || mkdir -p tests/atf/sh_interface
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/sh_interface/t_config.sh

atf_sh_interface_SCRIPTS += tests/atf/sh_interface/t_normalize
CLEANFILES += tests/atf/sh_interface/t_normalize
EXTRA_DIST += tests/atf/sh_interface/t_normalize.sh
tests/atf/sh_interface/t_normalize: \
		$(srcdir)/tests/atf/sh_interface/t_normalize.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/sh_interface || mkdir -p tests/atf/sh_interface
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/sh_interface/t_normalize.sh

atf_sh_interface_SCRIPTS += tests/atf/sh_interface/t_tc
CLEANFILES += tests/atf/sh_interface/t_tc
EXTRA_DIST += tests/atf/sh_interface/t_tc.sh
tests/atf/sh_interface/t_tc: \
		$(srcdir)/tests/atf/sh_interface/t_tc.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/sh_interface || mkdir -p tests/atf/sh_interface
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/sh_interface/t_tc.sh

atf_sh_interface_SCRIPTS += tests/atf/sh_interface/t_tp
CLEANFILES += tests/atf/sh_interface/t_tp
EXTRA_DIST += tests/atf/sh_interface/t_tp.sh
tests/atf/sh_interface/t_tp: \
		$(srcdir)/tests/atf/sh_interface/t_tp.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/sh_interface || mkdir -p tests/atf/sh_interface
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/sh_interface/t_tp.sh

atf_test_programs_DATA = tests/atf/test_programs/Atffile
atf_test_programsdir = $(pkgtestsdir)/test_programs
EXTRA_DIST += $(atf_test_programs_DATA)

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
		$(srcdir)/tests/atf/test_programs/t_cleanup.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/t_cleanup.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_config
CLEANFILES += tests/atf/test_programs/t_config
EXTRA_DIST += tests/atf/test_programs/t_config.sh
tests/atf/test_programs/t_config: \
		$(srcdir)/tests/atf/test_programs/t_config.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/t_config.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_env
CLEANFILES += tests/atf/test_programs/t_env
EXTRA_DIST += tests/atf/test_programs/t_env.sh
tests/atf/test_programs/t_env: \
		$(srcdir)/tests/atf/test_programs/t_env.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/t_env.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_fork
CLEANFILES += tests/atf/test_programs/t_fork
EXTRA_DIST += tests/atf/test_programs/t_fork.sh
tests/atf/test_programs/t_fork: \
		$(srcdir)/tests/atf/test_programs/t_fork.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/t_fork.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_meta_data
CLEANFILES += tests/atf/test_programs/t_meta_data
EXTRA_DIST += tests/atf/test_programs/t_meta_data.sh
tests/atf/test_programs/t_meta_data: \
		$(srcdir)/tests/atf/test_programs/t_meta_data.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ \
	    $(srcdir)/tests/atf/test_programs/t_meta_data.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_srcdir
CLEANFILES += tests/atf/test_programs/t_srcdir
EXTRA_DIST += tests/atf/test_programs/t_srcdir.sh
tests/atf/test_programs/t_srcdir: \
		$(srcdir)/tests/atf/test_programs/t_srcdir.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/t_srcdir.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_status
CLEANFILES += tests/atf/test_programs/t_status
EXTRA_DIST += tests/atf/test_programs/t_status.sh
tests/atf/test_programs/t_status: \
		$(srcdir)/tests/atf/test_programs/t_status.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/t_status.sh

atf_test_programs_SCRIPTS += tests/atf/test_programs/t_workdir
CLEANFILES += tests/atf/test_programs/t_workdir
EXTRA_DIST += tests/atf/test_programs/t_workdir.sh
tests/atf/test_programs/t_workdir: \
		$(srcdir)/tests/atf/test_programs/t_workdir.sh \
		$(ATF_COMPILE_DEPS)
	test -d tests/atf/test_programs || mkdir -p tests/atf/test_programs
	$(ATF_COMPILE_SH) -o $@ $(srcdir)/tests/atf/test_programs/t_workdir.sh

atf_test_programs_PROGRAMS = tests/atf/test_programs/h_cpp
tests_atf_test_programs_h_cpp_SOURCES = tests/atf/test_programs/h_cpp.cpp
tests_atf_test_programs_h_cpp_LDADD = -latf
tests_atf_test_programs_h_cpp_LDFLAGS = -L.

atf_tools_DATA = tests/atf/tools/Atffile
atf_toolsdir = $(pkgtestsdir)/tools
EXTRA_DIST += $(atf_tools_DATA)

atf_tools_PROGRAMS = tests/atf/tools/h_fail
tests_atf_tools_h_fail_SOURCES = tests/atf/tools/h_fail.cpp
tests_atf_tools_h_fail_LDADD = -latf
tests_atf_tools_h_fail_LDFLAGS = -L.

atf_tools_PROGRAMS += tests/atf/tools/h_misc
tests_atf_tools_h_misc_SOURCES = tests/atf/tools/h_misc.cpp
tests_atf_tools_h_misc_LDADD = -latf
tests_atf_tools_h_misc_LDFLAGS = -L.

atf_tools_PROGRAMS += tests/atf/tools/h_mode
tests_atf_tools_h_mode_SOURCES = tests/atf/tools/h_mode.cpp
tests_atf_tools_h_mode_LDADD = -latf
tests_atf_tools_h_mode_LDFLAGS = -L.

atf_tools_PROGRAMS += tests/atf/tools/h_pass
tests_atf_tools_h_pass_SOURCES = tests/atf/tools/h_pass.cpp
tests_atf_tools_h_pass_LDADD = -latf
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

atf_units_DATA = tests/atf/units/Atffile
atf_unitsdir = $(pkgtestsdir)/units
EXTRA_DIST += $(atf_units_DATA)

atf_units_PROGRAMS = tests/atf/units/t_config
tests_atf_units_t_config_SOURCES = tests/atf/units/t_config.cpp
tests_atf_units_t_config_LDADD = -latf
tests_atf_units_t_config_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_env
tests_atf_units_t_env_SOURCES = tests/atf/units/t_env.cpp
tests_atf_units_t_env_LDADD = -latf
tests_atf_units_t_env_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_expand
tests_atf_units_t_expand_SOURCES = tests/atf/units/t_expand.cpp
tests_atf_units_t_expand_LDADD = -latf
tests_atf_units_t_expand_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_fs
tests_atf_units_t_fs_SOURCES = tests/atf/units/t_fs.cpp
tests_atf_units_t_fs_LDADD = -latf
tests_atf_units_t_fs_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_io
tests_atf_units_t_io_SOURCES = tests/atf/units/t_io.cpp
tests_atf_units_t_io_LDADD = -latf
tests_atf_units_t_io_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_parser
tests_atf_units_t_parser_SOURCES = tests/atf/units/t_parser.cpp
tests_atf_units_t_parser_LDADD = -latf
tests_atf_units_t_parser_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_sanity
tests_atf_units_t_sanity_SOURCES = tests/atf/units/t_sanity.cpp
tests_atf_units_t_sanity_LDADD = -latf
tests_atf_units_t_sanity_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_signals
tests_atf_units_t_signals_SOURCES = tests/atf/units/t_signals.cpp
tests_atf_units_t_signals_LDADD = -latf
tests_atf_units_t_signals_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_tests
tests_atf_units_t_tests_SOURCES = tests/atf/units/t_tests.cpp
tests_atf_units_t_tests_LDADD = -latf
tests_atf_units_t_tests_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_text
tests_atf_units_t_text_SOURCES = tests/atf/units/t_text.cpp
tests_atf_units_t_text_LDADD = -latf
tests_atf_units_t_text_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_user
tests_atf_units_t_user_SOURCES = tests/atf/units/t_user.cpp
tests_atf_units_t_user_LDADD = -latf
tests_atf_units_t_user_LDFLAGS = -L.

atf_units_PROGRAMS += tests/atf/units/t_utils
tests_atf_units_t_utils_SOURCES = tests/atf/units/t_utils.cpp
tests_atf_units_t_utils_LDADD = -latf
tests_atf_units_t_utils_LDFLAGS = -L.

# -------------------------------------------------------------------------
# `tests/build' directory.
# -------------------------------------------------------------------------

check_LIBRARIES = tests/build/libuseless.a

tests_build_libuseless_a_SOURCES = \
    tests/build/t_include_application_hpp.cpp \
    tests/build/t_include_atf_hpp.cpp \
    tests/build/t_include_atffile_hpp.cpp \
    tests/build/t_include_config_hpp.cpp \
    tests/build/t_include_env_hpp.cpp \
    tests/build/t_include_exceptions_hpp.cpp \
    tests/build/t_include_expand_hpp.cpp \
    tests/build/t_include_formats_hpp.cpp \
    tests/build/t_include_fs_hpp.cpp \
    tests/build/t_include_io_hpp.cpp \
    tests/build/t_include_macros_hpp.cpp \
    tests/build/t_include_parser_hpp.cpp \
    tests/build/t_include_sanity_hpp.cpp \
    tests/build/t_include_signals_hpp.cpp \
    tests/build/t_include_tests_hpp.cpp \
    tests/build/t_include_text_hpp.cpp \
    tests/build/t_include_ui_hpp.cpp \
    tests/build/t_include_user_hpp.cpp \
    tests/build/t_include_utils_hpp.cpp \
    tests/build/t_use_macros_hpp.cpp

# -------------------------------------------------------------------------
# `tools' directory.
# -------------------------------------------------------------------------

bin_PROGRAMS = tools/atf-config
tools_atf_config_SOURCES = tools/atf-config.cpp
tools_atf_config_LDADD = -latf
tools_atf_config_LDFLAGS = -L.
dist_man_MANS += tools/atf-config.1

tools/atf-host-compile: $(srcdir)/tools/atf-host-compile.sh
	sed -e 's,__ATF_PKGDATADIR__,$(srcdir)/subrs,g' \
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
tools_atf_cleanup_LDADD = -latf
tools_atf_cleanup_LDFLAGS = -L.
dist_man_MANS += tools/atf-cleanup.1

bin_PROGRAMS += tools/atf-compile
tools_atf_compile_SOURCES = tools/atf-compile.cpp
tools_atf_compile_LDADD = -latf
tools_atf_compile_LDFLAGS = -L.
dist_man_MANS += tools/atf-compile.1

libexec_PROGRAMS += tools/atf-exec
tools_atf_exec_SOURCES = tools/atf-exec.cpp
tools_atf_exec_LDADD = -latf
tools_atf_exec_LDFLAGS = -L.
dist_man_MANS += tools/atf-exec.1

libexec_PROGRAMS += tools/atf-format
tools_atf_format_SOURCES = tools/atf-format.cpp
tools_atf_format_LDADD = -latf
tools_atf_format_LDFLAGS = -L.
dist_man_MANS += tools/atf-format.1

libexec_PROGRAMS += tools/atf-killpg
tools_atf_killpg_SOURCES = tools/atf-killpg.cpp
tools_atf_killpg_LDADD = -latf
tools_atf_killpg_LDFLAGS = -L.
dist_man_MANS += tools/atf-killpg.1

bin_PROGRAMS += tools/atf-report
tools_atf_report_SOURCES = tools/atf-report.cpp
tools_atf_report_LDADD = -latf
tools_atf_report_LDFLAGS = -L.
dist_man_MANS += tools/atf-report.1

bin_PROGRAMS += tools/atf-run
tools_atf_run_SOURCES = tools/atf-run.cpp
tools_atf_run_LDADD = -latf
tools_atf_run_LDFLAGS = -L.
dist_man_MANS += tools/atf-run.1

bin_PROGRAMS += tools/atf-version
tools_atf_version_SOURCES = tools/atf-version.cpp revision.h
tools_atf_version_LDADD = -latf
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
