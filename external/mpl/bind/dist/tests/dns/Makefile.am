include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=				\
	$(LIBISC_CFLAGS)		\
	$(LIBDNS_CFLAGS)		\
	$(LIBUV_CFLAGS)			\
	$(KRB5_CFLAGS)			\
	-DSRCDIR=\"$(abs_srcdir)\"	\
	-DBUILDDIR=\"$(abs_builddir)\"	\
	-I$(top_srcdir)/lib/dns		\
	-I$(top_srcdir)/lib/isc

LDADD +=				\
	$(LIBISC_LIBS)			\
	$(LIBUV_LIBS)			\
	$(LIBDNS_LIBS)

check_PROGRAMS =		\
	acl_test		\
	db_test			\
	dbdiff_test		\
	dbiterator_test		\
	dbversion_test		\
	dh_test			\
	dispatch_test		\
	dns64_test		\
	dst_test		\
	keytable_test		\
	name_test		\
	nsec3_test		\
	nsec3param_test		\
	private_test		\
	rbt_test		\
	rbtdb_test		\
	rdata_test		\
	rdataset_test		\
	rdatasetstats_test	\
	resolver_test		\
	rsa_test		\
	sigs_test		\
	time_test		\
	tsig_test		\
	update_test		\
	zonemgr_test		\
	zt_test

CLEANFILES =

if HAVE_PERL

check_PROGRAMS +=		\
	master_test

EXTRA_master_test_DEPENDENCIES = testdata/master/master12.data testdata/master/master13.data testdata/master/master14.data
CLEANFILES += $(EXTRA_master_test_DEPENDENCIES)

testdata/master/master12.data: testdata/master/master12.data.in
	mkdir -p testdata/master
	$(PERL) -w $(srcdir)/mkraw.pl < $(srcdir)/testdata/master/master12.data.in > $@

testdata/master/master13.data: testdata/master/master13.data.in
	mkdir -p testdata/master
	$(PERL) -w $(srcdir)/mkraw.pl < $(srcdir)/testdata/master/master13.data.in > $@

testdata/master/master14.data: testdata/master/master14.data.in
	mkdir -p testdata/master
	$(PERL) -w $(srcdir)/mkraw.pl < $(srcdir)/testdata/master/master14.data.in > $@

endif

if HAVE_GEOIP2
check_PROGRAMS +=		\
	geoip_test

geoip_test_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(MAXMINDDB_CFLAGS)	\
	-DTEST_GEOIP_DATA=\"$(top_srcdir)/bin/tests/system/geoip2/data\"

geoip_test_LDADD =		\
	$(LDADD)		\
	$(MAXMINDDB_LIBS)
endif

if HAVE_DNSTAP
check_PROGRAMS +=		\
	dnstap_test

dnstap_test_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(DNSTAP_CFLAGS)
dnstap_test_LDADD =		\
	$(LDADD)		\
	$(DNSTAP_LIBS)
endif

dh_test_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(OPENSSL_CFLAGS)

dst_test_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(OPENSSL_CFLAGS)

rsa_test_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(OPENSSL_CFLAGS)

rdata_test_CPPFLAGS = 		\
	$(AM_CPPFLAGS)		\
	$(OPENSSL_CFLAGS)

rdata_test_LDADD =		\
	$(LDADD)		\
	$(OPENSSL_LIBS)

EXTRA_sigs_test_DEPENDENCIES = testdata/master/master18.data
CLEANFILES += $(EXTRA_sigs_test_DEPENDENCIES)

testdata/master/master18.data: testdata/master/master18.data.in
	mkdir -p testdata/master
	sed -e 's,@TOP_SRCDIR@,$(abs_srcdir),' < $(srcdir)/testdata/master/master18.data.in > $@

EXTRA_DIST =			\
	Kdh.+002+18602.key	\
	Krsa.+008+29238.key	\
	comparekeys		\
	mkraw.pl		\
	testdata		\
	testkeys

include $(top_srcdir)/Makefile.tests
