SUBDIRS = .

AM_CPPFLAGS = $(ATF_CFLAGS) -DUNIT_TEST -I$(top_srcdir)/includes
AM_CPPFLAGS += -I$(top_srcdir)/bind/include -I$(top_srcdir)
AM_CPPFLAGS += -DLOCALSTATEDIR='"."'
AM_CPPFLAGS += -DCLIENT_PATH='"."'

EXTRA_DIST = Atffile duid0_test.txt duid1_test.txt duid2_test.txt duid3_test.txt

# for autotools debugging only
info:
	@echo "ATF_CFLAGS=$(ATF_CFLAGS)"
	@echo "ATF_LDFLAGS=$(ATF_LDFLAGS)"
	@echo "ATF_LIBS=$(ATF_LIBS)"

DHCPSRC = ../clparse.c ../dhc6.c ../dhclient.c

DHCPLIBS = $(top_builddir)/common/libdhcp.a $(top_builddir)/omapip/libomapi.a    \
          $(top_builddir)/dhcpctl/libdhcpctl.a $(top_builddir)/bind/lib/libirs.a \
	  $(top_builddir)/bind/lib/libdns.a $(top_builddir)/bind/lib/libisccfg.a \
	  $(top_builddir)/bind/lib/libisc.a

ATF_TESTS =
if HAVE_ATF

ATF_TESTS += duid_unittests 

duid_unittests_SOURCES = $(DHCPSRC) 
duid_unittests_SOURCES += duid_unittest.c

duid_unittests_LDADD = $(ATF_LDFLAGS)
duid_unittests_LDADD += $(DHCPLIBS)

check: $(ATF_TESTS)
	sh ${top_srcdir}/tests/unittest.sh

endif

check_PROGRAMS = $(ATF_TESTS)
