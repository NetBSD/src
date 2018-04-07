SUBDIRS = .

AM_CPPFLAGS = $(ATF_CFLAGS) -I$(top_srcdir)/includes

EXTRA_DIST = Atffile Kyuafile

ATF_TESTS =

if HAVE_ATF

ATF_TESTS += alloc_unittest dns_unittest misc_unittest ns_name_unittest \
	option_unittest

alloc_unittest_SOURCES = test_alloc.c $(top_srcdir)/tests/t_api_dhcp.c
alloc_unittest_LDADD = $(ATF_LDFLAGS)
alloc_unittest_LDADD += ../libdhcp.@A@ ../../omapip/libomapi.@A@ \
	@BINDLIBIRSDIR@/libirs.@A@ \
	@BINDLIBDNSDIR@/libdns.@A@ \
	@BINDLIBISCCFGDIR@/libisccfg.@A@  \
	@BINDLIBISCDIR@/libisc.@A@

dns_unittest_SOURCES = dns_unittest.c $(top_srcdir)/tests/t_api_dhcp.c
dns_unittest_LDADD = $(ATF_LDFLAGS)
dns_unittest_LDADD += ../libdhcp.@A@ ../../omapip/libomapi.@A@ \
	@BINDLIBIRSDIR@/libirs.@A@ \
	@BINDLIBDNSDIR@/libdns.@A@ \
	@BINDLIBISCCFGDIR@/libisccfg.@A@  \
	@BINDLIBISCDIR@/libisc.@A@

misc_unittest_SOURCES = misc_unittest.c $(top_srcdir)/tests/t_api_dhcp.c
misc_unittest_LDADD = $(ATF_LDFLAGS)
misc_unittest_LDADD += ../libdhcp.@A@ ../../omapip/libomapi.@A@ \
	@BINDLIBIRSDIR@/libirs.@A@ \
	@BINDLIBDNSDIR@/libdns.@A@ \
	@BINDLIBISCCFGDIR@/libisccfg.@A@  \
	@BINDLIBISCDIR@/libisc.@A@

ns_name_unittest_SOURCES = ns_name_test.c $(top_srcdir)/tests/t_api_dhcp.c
ns_name_unittest_LDADD = $(ATF_LDFLAGS)
ns_name_unittest_LDADD += ../libdhcp.@A@ ../../omapip/libomapi.@A@ \
	@BINDLIBIRSDIR@/libirs.@A@ \
	@BINDLIBDNSDIR@/libdns.@A@ \
	@BINDLIBISCCFGDIR@/libisccfg.@A@  \
	@BINDLIBISCDIR@/libisc.@A@

option_unittest_SOURCES = option_unittest.c $(top_srcdir)/tests/t_api_dhcp.c
option_unittest_LDADD = $(ATF_LDFLAGS)
option_unittest_LDADD += ../libdhcp.@A@ ../../omapip/libomapi.@A@ \
	@BINDLIBIRSDIR@/libirs.@A@ \
	@BINDLIBDNSDIR@/libdns.@A@ \
	@BINDLIBISCCFGDIR@/libisccfg.@A@  \
	@BINDLIBISCDIR@/libisc.@A@

check: $(ATF_TESTS)
	@if test $(top_srcdir) != ${top_builddir}; then \
		cp $(top_srcdir)/common/tests/Atffile Atffile; \
		cp $(top_srcdir)/common/tests/Kyuafile Kyuafile; \
	fi
	sh ${top_builddir}/tests/unittest.sh

distclean-local:
	@if test $(top_srcdir) != ${top_builddir}; then \
		rm -f Atffile Kyuafile; \
	fi

endif

check_PROGRAMS = $(ATF_TESTS)
