include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)

LDADD +=			\
	$(LIBDNS_LIBS)		\
	$(LIBISC_LIBS)

bin_PROGRAMS =			\
	arpaname		\
	mdig			\
	named-journalprint	\
	named-rrchecker		\
	nsec3hash

arpaname_LDADD =		\
	$(LIBISC_LIBS)

mdig_CPPFLAGS =			\
	$(AM_CPPFLAGS)		\
	$(LIBBIND9_CFLAGS)

mdig_LDADD =			\
	$(LIBBIND9_LIBS)	\
	$(LIBISCCFG_LIBS)	\
	$(LIBDNS_LIBS)		\
	$(LIBISC_LIBS)

if HAVE_DNSTAP
bin_PROGRAMS +=			\
	dnstap-read

dnstap_read_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(DNSTAP_CFLAGS)	\
	-I$(top_builddir)/lib/dns

dnstap_read_LDADD =		\
	$(LIBDNS_LIBS)		\
	$(LIBISC_LIBS)		\
	$(DNSTAP_LIBS)
endif

if HAVE_LMDB
bin_PROGRAMS +=			\
	named-nzd2nzf

named_nzd2nzf_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(LMDB_CFLAGS)

named_nzd2nzf_LDADD =		\
	$(LIBISC_LIBS)		\
	$(LMDB_LIBS)

endif
