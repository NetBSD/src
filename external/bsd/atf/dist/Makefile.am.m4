#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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
# Generic macros.
# -------------------------------------------------------------------------

m4_changequote([, ])

# AUTOMAKE_ID filename
#
# Converts a filename to an Automake identifier, i.e. a string that can be
# used as part of a variable name.
m4_define([AUTOMAKE_ID], [m4_translit([$1], [-/.+], [____])])

# DO_ONCE id what
#
# Do 'what' only once, even if called multiple times.  Uses 'id' to identify
# this specific 'what'.
m4_define([DO_ONCE], [m4_ifdef([done_$1], [], [$2 m4_define(done_$1, [])])])

# INIT_VAR var
#
# Initializes a variable to the empty string the first time it is called.
m4_define([INIT_VAR], [DO_ONCE($1, [$1 =])])

# Explicitly initialize some variables so that we can use += later on
# without making Automake complain.
CLEANFILES =
EXTRA_DIST =

# -------------------------------------------------------------------------
# Top directory.
# -------------------------------------------------------------------------

EXTRA_DIST += Makefile.am.m4
$(srcdir)/Makefile.am: $(srcdir)/admin/generate-makefile.sh \
                       $(srcdir)/Makefile.am.m4
	$(srcdir)/admin/generate-makefile.sh \
	    $(srcdir)/Makefile.am.m4 $(srcdir)/Makefile.am

doc_DATA = AUTHORS COPYING NEWS README
noinst_DATA = INSTALL
EXTRA_DIST += $(doc_DATA)

dist-hook: $(srcdir)/admin/revision-dist.h check-install check-style

AM_CPPFLAGS = "-DATF_ARCH=\"$(atf_arch)\"" \
              "-DATF_BUILD_CC=\"$(ATF_BUILD_CC)\"" \
              "-DATF_BUILD_CFLAGS=\"$(ATF_BUILD_CFLAGS)\"" \
              "-DATF_BUILD_CPP=\"$(ATF_BUILD_CPP)\"" \
              "-DATF_BUILD_CPPFLAGS=\"$(ATF_BUILD_CPPFLAGS)\"" \
              "-DATF_BUILD_CXX=\"$(ATF_BUILD_CXX)\"" \
              "-DATF_BUILD_CXXFLAGS=\"$(ATF_BUILD_CXXFLAGS)\"" \
              "-DATF_CONFDIR=\"$(atf_confdir)\"" \
              "-DATF_INCLUDEDIR=\"$(includedir)\"" \
              "-DATF_LIBDIR=\"$(libdir)\"" \
              "-DATF_LIBEXECDIR=\"$(libexecdir)\"" \
              "-DATF_MACHINE=\"$(atf_machine)\"" \
              "-DATF_M4=\"$(ATF_M4)\"" \
              "-DATF_PKGDATADIR=\"$(pkgdatadir)\"" \
              "-DATF_SHELL=\"$(ATF_SHELL)\"" \
              "-DATF_WORKDIR=\"$(ATF_WORKDIR)\""

ATF_COMPILE_DEPS = $(srcdir)/atf-sh/atf.init.subr
ATF_COMPILE_DEPS += tools/atf-host-compile
ATF_COMPILE_SH = ./tools/atf-host-compile

# DISTFILE_DOC name src
#
# Generates a rule to generate the prebuilt copy of the top-level document
# indicated in 'name' based on a generated document pointed to by 'src'.
m4_define([DISTFILE_DOC], [
$(srcdir)/$1: $(srcdir)/$2
	@if cmp -s $(srcdir)/$2 $(srcdir)/$1; then \
	    :; \
	else \
	    echo cp $(srcdir)/$2 $(srcdir)/$1; \
	    cp $(srcdir)/$2 $(srcdir)/$1; \
	fi
])

DISTFILE_DOC([AUTHORS], [doc/text/authors.txt])
DISTFILE_DOC([COPYING], [doc/text/copying.txt])
DISTFILE_DOC([INSTALL], [doc/text/install.txt])
DISTFILE_DOC([NEWS], [doc/text/news.txt])
DISTFILE_DOC([README], [doc/text/readme.txt])

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
              admin/check-style.sh \
              admin/choose-revision.sh \
              admin/generate-makefile.sh \
              admin/generate-revision.sh \
              admin/generate-revision-dist.sh

# REVISION_FILE fmt
#
# Generates rules to create the revision files for the given format.
# These create admin/revision.<fmt> and $(srcdir)/admin/revision-dist.<fmt>
m4_define([REVISION_FILE], [
.PHONY: admin/revision.$1
admin/revision.$1:
	@$(top_srcdir)/admin/generate-revision.sh \
	    -f $1 -m "$(MTN)" -r $(top_srcdir) -o admin/revision.$1 \
	    -v $(PACKAGE_VERSION)
CLEANFILES += admin/revision.$1

$(srcdir)/admin/revision-dist.$1: admin/revision.$1
	@$(top_srcdir)/admin/generate-revision-dist.sh \
	    -f $1 -i admin/revision.$1 -o $(srcdir)/admin/revision-dist.$1
EXTRA_DIST += admin/revision-dist.$1
])

REVISION_FILE([h])

# -------------------------------------------------------------------------
# `atf-c' directory.
# -------------------------------------------------------------------------

lib_LTLIBRARIES = libatf-c.la
libatf_c_la_SOURCES = atf-c/build.c \
                      atf-c/build.h \
                      atf-c/check.c \
                      atf-c/check.h \
                      atf-c/config.c \
                      atf-c/config.h \
                      atf-c/dynstr.c \
                      atf-c/dynstr.h \
                      atf-c/env.c \
                      atf-c/env.h \
                      atf-c/error.c \
                      atf-c/error.h \
                      atf-c/error_fwd.h \
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
                      atf-c/process.c \
                      atf-c/process.h \
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
atf_c_HEADERS = atf-c/build.h \
                atf-c/check.h \
                atf-c/config.h \
                atf-c/defs.h \
                atf-c/dynstr.h \
                atf-c/env.h \
                atf-c/error.h \
                atf-c/error_fwd.h \
                atf-c/expand.h \
                atf-c/fs.h \
                atf-c/io.h \
                atf-c/list.h \
                atf-c/macros.h \
                atf-c/map.h \
                atf-c/object.h \
                atf-c/process.h \
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
libatf_c___la_LIBADD = libatf-c.la
libatf_c___la_SOURCES = atf-c++/application.cpp \
                        atf-c++/application.hpp \
                        atf-c++/atffile.cpp \
                        atf-c++/atffile.hpp \
                        atf-c++/build.cpp \
                        atf-c++/build.hpp \
                        atf-c++/check.cpp \
                        atf-c++/check.hpp \
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
                        atf-c++/process.cpp \
                        atf-c++/process.hpp \
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
                  atf-c++/build.hpp \
                  atf-c++/check.hpp \
                  atf-c++/config.hpp \
                  atf-c++/env.hpp \
                  atf-c++/exceptions.hpp \
                  atf-c++/expand.hpp \
                  atf-c++/formats.hpp \
                  atf-c++/fs.hpp \
                  atf-c++/io.hpp \
                  atf-c++/macros.hpp \
                  atf-c++/parser.hpp \
                  atf-c++/process.hpp \
                  atf-c++/sanity.hpp \
                  atf-c++/signals.hpp \
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

_STANDALONE_XSLT = doc/standalone/sdocbook.xsl

EXTRA_DIST += doc/build-xml.sh
EXTRA_DIST += doc/standalone/standalone.css
EXTRA_DIST += $(_STANDALONE_XSLT)

BUILD_XML_ENV = DOC_BUILD=$(DOC_BUILD) \
                LINKS=$(LINKS) \
                TIDY=$(TIDY) \
                XML_CATALOG_FILE=$(XML_CATALOG_FILE) \
                XMLLINT=$(XMLLINT) \
                XSLTPROC=$(XSLTPROC)

# XML_DOC basename
#
# Formats doc/<basename>.xml into HTML and plain text versions.
m4_define([XML_DOC], [
EXTRA_DIST += doc/$1.xml
EXTRA_DIST += doc/standalone/$1.html
noinst_DATA += doc/standalone/$1.html
EXTRA_DIST += doc/text/$1.txt
noinst_DATA += doc/text/$1.txt
doc/standalone/$1.html: $(srcdir)/doc/$1.xml doc/build-xml.sh \
                        $(_STANDALONE_XSLT)
	$(BUILD_XML_ENV) $(ATF_SHELL) doc/build-xml.sh \
	    $(srcdir)/doc/$1.xml $(srcdir)/$(_STANDALONE_XSLT) \
	    html:$(srcdir)/doc/standalone/$1.html
doc/text/$1.txt: $(srcdir)/doc/$1.xml doc/build-xml.sh $(_STANDALONE_XSLT)
	$(BUILD_XML_ENV) $(ATF_SHELL) doc/build-xml.sh \
	    $(srcdir)/doc/$1.xml $(srcdir)/$(_STANDALONE_XSLT) \
	    txt:$(srcdir)/doc/text/$1.txt
])

XML_DOC([authors])
XML_DOC([copying])
XML_DOC([install])
XML_DOC([news])
XML_DOC([readme])
XML_DOC([roadmap])
XML_DOC([specification])

# -------------------------------------------------------------------------
# `m4' directory.
# -------------------------------------------------------------------------

ACLOCAL_AMFLAGS = -I m4

# -------------------------------------------------------------------------
# `tests/bootstrap' directory.
# -------------------------------------------------------------------------

check_PROGRAMS = tests/bootstrap/h_app_empty
tests_bootstrap_h_app_empty_SOURCES = tests/bootstrap/h_app_empty.cpp
tests_bootstrap_h_app_empty_LDADD = libatf-c++.la

check_PROGRAMS += tests/bootstrap/h_app_opts_args
tests_bootstrap_h_app_opts_args_SOURCES = tests/bootstrap/h_app_opts_args.cpp
tests_bootstrap_h_app_opts_args_LDADD = libatf-c++.la

check_PROGRAMS += tests/bootstrap/h_tp_basic_c
tests_bootstrap_h_tp_basic_c_SOURCES = tests/bootstrap/h_tp_basic_c.c
tests_bootstrap_h_tp_basic_c_LDADD = libatf-c.la

check_PROGRAMS += tests/bootstrap/h_tp_basic_cpp
tests_bootstrap_h_tp_basic_cpp_SOURCES = tests/bootstrap/h_tp_basic_cpp.cpp
tests_bootstrap_h_tp_basic_cpp_LDADD = libatf-c++.la

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
	echo 'm4_[]define(AT_PACKAGE_NAME,      @PACKAGE_NAME@)'; \
	echo 'm4_[]define(AT_PACKAGE_TARNAME,   @PACKAGE_TARNAME@)'; \
	echo 'm4_[]define(AT_PACKAGE_VERSION,   @PACKAGE_VERSION@)'; \
	echo 'm4_[]define(AT_PACKAGE_STRING,    @PACKAGE_STRING@)'; \
	echo 'm4_[]define(AT_PACKAGE_BUGREPORT, @PACKAGE_BUGREPORT@)'; \
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

TESTS_ENVIRONMENT = PATH=$(prefix)/bin:$${PATH} \
                    PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig

installcheck-local: installcheck-bootstrap installcheck-atf

# TODO: This really needs to be a 'check' target and not 'installcheck', but
# these bootstrap tests don't currently work without atf being installed.
.PHONY: installcheck-bootstrap
installcheck-bootstrap: $(srcdir)/tests/bootstrap/testsuite check
	$(TESTS_ENVIRONMENT) $(srcdir)/tests/bootstrap/testsuite

.PHONY: installcheck-atf
installcheck-atf:
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

atf_atf_c_DATA = tests/atf/atf-c/Atffile \
                 tests/atf/atf-c/d_include_atf_c_h.c \
                 tests/atf/atf-c/d_include_build_h.c \
                 tests/atf/atf-c/d_include_check_h.c \
                 tests/atf/atf-c/d_include_config_h.c \
                 tests/atf/atf-c/d_include_dynstr_h.c \
                 tests/atf/atf-c/d_include_env_h.c \
                 tests/atf/atf-c/d_include_error_fwd_h.c \
                 tests/atf/atf-c/d_include_error_h.c \
                 tests/atf/atf-c/d_include_expand_h.c \
                 tests/atf/atf-c/d_include_fs_h.c \
                 tests/atf/atf-c/d_include_io_h.c \
                 tests/atf/atf-c/d_include_list_h.c \
                 tests/atf/atf-c/d_include_macros_h.c \
                 tests/atf/atf-c/d_include_map_h.c \
                 tests/atf/atf-c/d_include_object_h.c \
                 tests/atf/atf-c/d_include_process_h.c \
                 tests/atf/atf-c/d_include_sanity_h.c \
                 tests/atf/atf-c/d_include_signals_h.c \
                 tests/atf/atf-c/d_include_tc_h.c \
                 tests/atf/atf-c/d_include_tcr_h.c \
                 tests/atf/atf-c/d_include_text_h.c \
                 tests/atf/atf-c/d_include_tp_h.c \
                 tests/atf/atf-c/d_include_ui_h.c \
                 tests/atf/atf-c/d_include_user_h.c \
                 tests/atf/atf-c/d_use_macros_h.c
atf_atf_cdir = $(pkgtestsdir)/atf-c
EXTRA_DIST += $(atf_atf_c_DATA)

noinst_LTLIBRARIES = tests/atf/atf-c/libh.la
tests_atf_atf_c_libh_la_SOURCES = tests/atf/atf-c/h_lib.c \
                                  tests/atf/atf-c/h_lib.h

# C_TP subdir progname extradeps extralibs
#
# Generates rules to build a C test program.  The 'subdir' is relative to
# tests/ and progname is the source file name without .c.
m4_define([C_TP], [
INIT_VAR(AUTOMAKE_ID([$1])_PROGRAMS)
AUTOMAKE_ID([$1])_PROGRAMS += tests/$1/$2
tests_[]AUTOMAKE_ID([$1_$2])_SOURCES = tests/$1/$2.c $3
tests_[]AUTOMAKE_ID([$1_$2])_LDADD = $4 libatf-c.la
])

# CXX_TP subdir progname extradeps extralibs
#
# Generates rules to build a C test program.  The 'subdir' is relative to
# tests/ and progname is the source file name without .c.
m4_define([CXX_TP], [
INIT_VAR(AUTOMAKE_ID([$1])_PROGRAMS)
AUTOMAKE_ID([$1])_PROGRAMS += tests/$1/$2
tests_[]AUTOMAKE_ID([$1_$2])_SOURCES = tests/$1/$2.cpp $3
tests_[]AUTOMAKE_ID([$1_$2])_LDADD = $4 libatf-c++.la
])

# SH_TP subdir progname extradeps
#
# Generates rules to build a C test program.  The 'subdir' is relative to
# tests/ and progname is the source file name without .c.
m4_define([SH_TP], [
INIT_VAR(AUTOMAKE_ID([$1])_SCRIPTS)
AUTOMAKE_ID([$1])_SCRIPTS += tests/$1/$2
CLEANFILES += tests/$1/$2
EXTRA_DIST += tests/$1/$2.sh
tests/$1/$2: $(srcdir)/tests/$1/$2.sh $(ATF_COMPILE_DEPS)
	test -d tests/$1 || mkdir -p tests/$1
	$(ATF_COMPILE_SH) -o tests/$1/$2 $(srcdir)/tests/$1/$2.sh
])

C_TP([atf/atf-c], [t_atf_c], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_build], [tests/atf/atf-c/h_build.h],
     [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_check], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_config], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_dynstr], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_env], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_error], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_expand], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_fs], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_h_lib], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_io], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_list], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_macros], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_map], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_process], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_signals], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_tc], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_tcr], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_sanity], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_text], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_ui], [], [tests/atf/atf-c/libh.la])
C_TP([atf/atf-c], [t_user], [], [tests/atf/atf-c/libh.la])

atf_atf_c_PROGRAMS += tests/atf/atf-c/h_processes
tests_atf_atf_c_h_processes_SOURCES = tests/atf/atf-c/h_processes.c

atf_atf_c___DATA = tests/atf/atf-c++/Atffile \
                   tests/atf/atf-c++/d_include_application_hpp.cpp \
                   tests/atf/atf-c++/d_include_atf_c++_hpp.cpp \
                   tests/atf/atf-c++/d_include_atffile_hpp.cpp \
                   tests/atf/atf-c++/d_include_build_hpp.cpp \
                   tests/atf/atf-c++/d_include_check_hpp.cpp \
                   tests/atf/atf-c++/d_include_config_hpp.cpp \
                   tests/atf/atf-c++/d_include_env_hpp.cpp \
                   tests/atf/atf-c++/d_include_exceptions_hpp.cpp \
                   tests/atf/atf-c++/d_include_expand_hpp.cpp \
                   tests/atf/atf-c++/d_include_formats_hpp.cpp \
                   tests/atf/atf-c++/d_include_fs_hpp.cpp \
                   tests/atf/atf-c++/d_include_io_hpp.cpp \
                   tests/atf/atf-c++/d_include_macros_hpp.cpp \
                   tests/atf/atf-c++/d_include_parser_hpp.cpp \
                   tests/atf/atf-c++/d_include_process_hpp.cpp \
                   tests/atf/atf-c++/d_include_sanity_hpp.cpp \
                   tests/atf/atf-c++/d_include_signals_hpp.cpp \
                   tests/atf/atf-c++/d_include_tests_hpp.cpp \
                   tests/atf/atf-c++/d_include_text_hpp.cpp \
                   tests/atf/atf-c++/d_include_ui_hpp.cpp \
                   tests/atf/atf-c++/d_include_user_hpp.cpp \
                   tests/atf/atf-c++/d_include_utils_hpp.cpp \
                   tests/atf/atf-c++/d_use_macros_hpp.cpp
atf_atf_c__dir = $(pkgtestsdir)/atf-c++
EXTRA_DIST += $(atf_atf_c___DATA)

noinst_LTLIBRARIES += tests/atf/atf-c++/libh.la
tests_atf_atf_c___libh_la_SOURCES = tests/atf/atf-c++/h_lib.cpp \
                                    tests/atf/atf-c++/h_lib.hpp

CXX_TP([atf/atf-c++], [t_atf_c++], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_application], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_atffile], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_build], [tests/atf/atf-c/h_build.h],
       [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_check], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_config], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_env], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_exceptions], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_expand], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_formats], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_fs], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_io], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_macros], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_parser], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_process], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_sanity], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_signals], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_tests], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_text], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_ui], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_user], [], [tests/atf/atf-c++/libh.la])
CXX_TP([atf/atf-c++], [t_utils], [], [tests/atf/atf-c++/libh.la])

atf_atf_sh_DATA = tests/atf/atf-sh/Atffile
atf_atf_shdir = $(pkgtestsdir)/atf-sh
EXTRA_DIST += $(atf_atf_sh_DATA)

SH_TP([atf/atf-sh], [h_misc])
SH_TP([atf/atf-sh], [t_atf_check])
SH_TP([atf/atf-sh], [t_config])
SH_TP([atf/atf-sh], [t_normalize])
SH_TP([atf/atf-sh], [t_tc])
SH_TP([atf/atf-sh], [t_tp])

atf_data_DATA = tests/atf/data/Atffile
atf_datadir = $(pkgtestsdir)/data
EXTRA_DIST += $(atf_data_DATA)

SH_TP([atf/data], [t_pkg_config])

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

CXX_TP([atf/formats], [h_parser])
CXX_TP([atf/formats], [t_writers])

SH_TP([atf/formats], [t_parsers])

atf_test_programs_DATA = tests/atf/test_programs/Atffile
atf_test_programsdir = $(pkgtestsdir)/test_programs
EXTRA_DIST += $(atf_test_programs_DATA)

EXTRA_DIST += tests/atf/test_programs/common.sh

C_TP([atf/test_programs], [h_c], [], [tests/atf/atf-c/libh.la])
CXX_TP([atf/test_programs], [h_cpp], [], [tests/atf/atf-c++/libh.la])
SH_TP([atf/test_programs], [h_sh])
SH_TP([atf/test_programs], [t_cleanup])
SH_TP([atf/test_programs], [t_config])
SH_TP([atf/test_programs], [t_env])
SH_TP([atf/test_programs], [t_fork])
SH_TP([atf/test_programs], [t_meta_data])
SH_TP([atf/test_programs], [t_srcdir])
SH_TP([atf/test_programs], [t_status])
SH_TP([atf/test_programs], [t_workdir])

atf_tools_DATA = tests/atf/tools/Atffile
atf_toolsdir = $(pkgtestsdir)/tools
EXTRA_DIST += $(atf_tools_DATA)

CXX_TP([atf/tools], [h_fail])
CXX_TP([atf/tools], [h_misc])
CXX_TP([atf/tools], [h_mode])
CXX_TP([atf/tools], [h_pass])
SH_TP([atf/tools], [t_atf_check])
SH_TP([atf/tools], [t_atf_cleanup])
SH_TP([atf/tools], [t_atf_compile])
SH_TP([atf/tools], [t_atf_config])
SH_TP([atf/tools], [t_atf_exec])
SH_TP([atf/tools], [t_atf_report])
SH_TP([atf/tools], [t_atf_run])

# -------------------------------------------------------------------------
# `tools' directory.
# -------------------------------------------------------------------------

# TOOL dir basename extradeps
#
# Builds a binary tool named 'basename/ and to be installed in 'dir'.
m4_define([TOOL], [
INIT_VAR([$1_PROGRAMS])
$1_PROGRAMS += tools/$2
tools_[]AUTOMAKE_ID([$2])_SOURCES = tools/$2.cpp $3
tools_[]AUTOMAKE_ID([$2])_LDADD = libatf-c++.la
dist_man_MANS += tools/$2.1
])

TOOL([bin], [atf-check])
TOOL([bin], [atf-config])
TOOL([libexec], [atf-cleanup])
TOOL([bin], [atf-compile])
TOOL([libexec], [atf-exec])
TOOL([libexec], [atf-format])
TOOL([bin], [atf-report])
TOOL([bin], [atf-run])
TOOL([bin], [atf-version], [revision.h])

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

BUILT_SOURCES = revision.h
revision.h: admin/revision.h $(srcdir)/admin/revision-dist.h
	@$(top_srcdir)/admin/choose-revision.sh \
	    admin/revision.h $(srcdir)/admin/revision-dist.h revision.h
CLEANFILES += revision.h

hooksdir = $(pkgdatadir)
hooks_DATA = tools/atf-run.hooks
EXTRA_DIST += $(hooks_DATA)

# vim: syntax=make:noexpandtab:shiftwidth=8:softtabstop=8
