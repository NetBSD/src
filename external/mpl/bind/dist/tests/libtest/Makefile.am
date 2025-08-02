include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	$(LIBNS_CFLAGS)		\
	$(LIBURCU_CFLAGS)	\
	$(LIBUV_CFLAGS)		\
	-I$(top_srcdir)/lib/isc	\
	-I$(top_srcdir)/lib/dns

LDADD +=			\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)		\
	$(LIBNS_LIBS)

noinst_LTLIBRARIES = libtest.la

libtest_la_SOURCES =			\
	../include/tests/isc.h		\
	../include/tests/ns.h		\
	../include/tests/qp.h		\
	isc.c				\
	ns.c				\
	qp.c
if HAVE_CMOCKA
libtest_la_SOURCES +=			\
	../include/tests/dns.h		\
        dns.c
endif HAVE_CMOCKA

include $(top_srcdir)/Makefile.tests
