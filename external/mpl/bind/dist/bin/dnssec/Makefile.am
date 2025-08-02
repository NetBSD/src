include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBISCCFG_CFLAGS)	\
	$(LIBDNS_CFLAGS)

AM_CPPFLAGS +=			\
	-DNAMED_CONFFILE=\"${sysconfdir}/named.conf\"

noinst_LTLIBRARIES = libdnssectool.la

LDADD +=			\
	libdnssectool.la	\
	$(LIBISC_LIBS)		\
	$(LIBISCCFG_LIBS)	\
	$(LIBDNS_LIBS)		\
	$(OPENSSL_LIBS)

bin_PROGRAMS = \
	dnssec-cds		\
	dnssec-dsfromkey	\
	dnssec-importkey	\
	dnssec-keyfromlabel	\
	dnssec-keygen		\
	dnssec-ksr		\
	dnssec-revoke		\
	dnssec-settime		\
	dnssec-signzone		\
	dnssec-verify

libdnssectool_la_SOURCES =	\
	dnssectool.h		\
	dnssectool.c

dnssec_keygen_CPPFLAGS =	\
	$(AM_CPPFLAGS)		\
	$(OPENSSL_CFLAGS)

dnssec_keygen_LDADD =		\
	$(LDADD)		\
	$(OPENSSL_LIBS)

dnssec_signzone_CPPFLAGS =	\
	$(AM_CPPFLAGS)		\
	$(OPENSSL_CFLAGS)

dnssec_signzone_LDADD =		\
	$(LDADD)		\
	$(OPENSSL_LIBS)
