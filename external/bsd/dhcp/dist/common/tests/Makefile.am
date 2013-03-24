SUBDIRS = .

AM_CPPFLAGS = $(ATF_CFLAGS) -I$(top_srcdir)/includes

EXTRA_DIST = Atffile

ATF_TESTS =

if HAVE_ATF

ATF_TESTS += alloc_unittest

alloc_unittest_SOURCES = test_alloc.c $(top_srcdir)/tests/t_api_dhcp.c
alloc_unittest_LDADD = $(ATF_LDFLAGS)
alloc_unittest_LDADD += ../libdhcp.a  \
	../../omapip/libomapi.a ../../bind/lib/libdns.a \
        ../../bind/lib/libisc.a

check: $(ATF_TESTS)
	atf-run | atf-report

endif

check_PROGRAMS = $(ATF_TESTS)
