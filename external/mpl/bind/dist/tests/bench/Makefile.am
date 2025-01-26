include $(top_srcdir)/Makefile.top

AM_CFLAGS += -Wno-vla

AM_CPPFLAGS +=				\
	$(LIBUV_CFLAGS)			\
	$(LIBURCU_CFLAGS)		\
	$(LIBISC_CFLAGS)		\
	$(LIBDNS_CFLAGS)		\
	-I$(top_srcdir)/fuzz		\
	-I$(top_srcdir)/lib/dns		\
	-I$(top_srcdir)/lib/isc		\
	-I$(top_srcdir)/tests/include

LDADD +=				\
	$(LIBUV_LIBS)			\
	$(LIBURCU_LIBS)			\
	$(LIBISC_LIBS)			\
	$(LIBDNS_LIBS)			\
	$(top_builddir)/tests/libtest/libtest.la

noinst_PROGRAMS =			\
	ascii				\
	compress			\
	dns_name_fromwire		\
	iterated_hash			\
	load-names			\
	qp-dump				\
	qplookups			\
	qpmulti				\
	siphash

dns_name_fromwire_SOURCES =		\
	$(top_builddir)/fuzz/old.c	\
	$(top_builddir)/fuzz/old.h	\
	dns_name_fromwire.c
