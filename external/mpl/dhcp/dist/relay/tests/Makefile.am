SUBDIRS = .

AM_CPPFLAGS = $(ATF_CFLAGS) -DUNIT_TEST -I$(top_srcdir)/includes
AM_CPPFLAGS += -I@BINDDIR@/include -I$(top_srcdir)
AM_CPPFLAGS += -DLOCALSTATEDIR='"."'

EXTRA_DIST = Atffile Kyuafile

# for autotools debugging only
info:
	@echo "ATF_CFLAGS=$(ATF_CFLAGS)"
	@echo "ATF_LDFLAGS=$(ATF_LDFLAGS)"
	@echo "ATF_LIBS=$(ATF_LIBS)"

DHCPSRC = ../dhcrelay.c

DHCPLIBS = $(top_builddir)/common/libdhcp.@A@ \
	  $(top_builddir)/omapip/libomapi.@A@    \
	  @BINDLIBIRSDIR@/libirs.@A@ \
	  @BINDLIBDNSDIR@/libdns.@A@ \
	  @BINDLIBISCCFGDIR@/libisccfg.@A@ \
	  @BINDLIBISCDIR@/libisc.@A@

ATF_TESTS =
if HAVE_ATF

ATF_TESTS += relay_unittests

relay_unittests_SOURCES = $(DHCPSRC)
relay_unittests_SOURCES += relay_unittests.c

relay_unittests_LDADD = $(ATF_LDFLAGS)
relay_unittests_LDADD += $(DHCPLIBS)

check: $(ATF_TESTS)
	@if test $(top_srcdir) != ${top_builddir}; then \
		cp $(top_srcdir)/relay/tests/Atffile Atffile; \
		cp $(top_srcdir)/relay/tests/Kyuafile Kyuafile; \
	fi
	sh ${top_builddir}/tests/unittest.sh

distclean-local:
	@if test $(top_srcdir) != ${top_builddir}; then \
		rm -f Atffile Kyuafile; \
	fi

endif

check_PROGRAMS = $(ATF_TESTS)
