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

DHCPSRC = ../dhcp.c ../bootp.c ../confpars.c ../db.c ../class.c      \
          ../failover.c ../omapi.c ../mdb.c ../stables.c ../salloc.c \
          ../ddns.c ../dhcpleasequery.c ../dhcpv6.c ../mdb6.c        \
          ../ldap.c ../ldap_casa.c ../dhcpd.c ../leasechain.c

DHCPLIBS = $(top_builddir)/common/libdhcp.@A@ \
	  $(top_builddir)/omapip/libomapi.@A@ \
          $(top_builddir)/dhcpctl/libdhcpctl.@A@ \
	  $(BINDLIBIRSDIR)/libirs.@A@ \
	  $(BINDLIBDNSDIR)/libdns.@A@ \
	  $(BINDLIBISCCFGDIR)/libisccfg.@A@ \
	  $(BINDLIBISCDIR)/libisc.@A@

ATF_TESTS =
if HAVE_ATF

ATF_TESTS += dhcpd_unittests legacy_unittests hash_unittests load_bal_unittests leaseq_unittests

dhcpd_unittests_SOURCES = $(DHCPSRC)
dhcpd_unittests_SOURCES += simple_unittest.c

dhcpd_unittests_LDADD = $(ATF_LDFLAGS)
dhcpd_unittests_LDADD += $(DHCPLIBS)

dhcpd_unittests_LDFLAGS = $(AM_LDFLAGS) $(ATF_LDFLAGS)

hash_unittests_SOURCES = $(DHCPSRC) hash_unittest.c
hash_unittests_LDADD = $(DHCPLIBS) $(ATF_LDFLAGS)

# This is a legacy unittest. It replaces main() with something that was in mdb6.c
legacy_unittests_SOURCES = $(DHCPSRC) mdb6_unittest.c
legacy_unittests_LDADD = $(DHCPLIBS) $(ATF_LDFLAGS)

load_bal_unittests_SOURCES = $(DHCPSRC) load_bal_unittest.c
load_bal_unittests_LDADD = $(DHCPLIBS) $(ATF_LDFLAGS)

leaseq_unittests_SOURCES = $(DHCPSRC) leaseq_unittest.c
leaseq_unittests_LDADD = $(DHCPLIBS) $(ATF_LDFLAGS)

check: $(ATF_TESTS)
	@if test $(top_srcdir) != ${top_builddir}; then \
		cp $(top_srcdir)/server/tests/Atffile Atffile; \
		cp $(top_srcdir)/server/tests/Kyuafile Kyuafile; \
	fi
	sh ${top_builddir}/tests/unittest.sh

distclean-local:
	@if test $(top_srcdir) != ${top_builddir}; then \
		rm -f Atffile Kyuafile; \
	fi

endif

check_PROGRAMS = $(ATF_TESTS)
