include $(top_srcdir)/Makefile.top
include $(top_srcdir)/Makefile.docs

OPTIONS_FILES =				\
	rndc.grammar			\
	options				\
	primary.zoneopt			\
	secondary.zoneopt			\
	mirror.zoneopt			\
	forward.zoneopt			\
	hint.zoneopt			\
	stub.zoneopt			\
	static-stub.zoneopt		\
	redirect.zoneopt		\
	delegation-only.zoneopt		\
	in-view.zoneopt

EXTRA_DIST =			\
	$(OPTIONS_FILES)	\
	checkgrammar.py		\
	parsegrammar.py		\
	sort-options.pl

if MAINTAINER_MODE

MAINTAINERCLEANFILES = $(OPTIONS_FILES)

noinst_PROGRAMS = cfg_test

cfg_test_CPPFLAGS =			\
	$(AM_CPPFLAGS)			\
	$(LIBISC_CFLAGS)		\
	$(LIBDNS_CFLAGS)		\
	$(LIBISCCFG_CFLAGS)

cfg_test_LDADD =			\
	$(LIBISC_LIBS)			\
	$(LIBDNS_LIBS)			\
	$(LIBISCCFG_LIBS)

BUILT_SOURCES =				\
	$(OPTIONS_FILES)

rndc.grammar: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --rndc --grammar | $(PERL) $(srcdir)/sort-options.pl > $@

options: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --named --grammar | $(PERL) $(srcdir)/sort-options.pl > $@

primary.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar primary > $@

secondary.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar secondary > $@

mirror.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar mirror > $@

forward.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar forward > $@

hint.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar hint > $@

stub.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar stub > $@

static-stub.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar static-stub > $@

redirect.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar redirect > $@

delegation-only.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar delegation-only > $@

in-view.zoneopt: cfg_test
	$(AM_V_CFG_TEST)$(builddir)/cfg_test --zonegrammar in-view > $@

endif
