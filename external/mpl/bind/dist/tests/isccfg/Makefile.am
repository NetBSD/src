include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	$(LIBISCCFG_CFLAGS)	\
	$(LIBUV_CFLAGS)		\
	-I$(top_srcdir)/lib/isc

LDADD +=			\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)		\
	$(LIBISCCFG_LIBS)	\
	$(LIBUV_LIBS)

check_PROGRAMS =		\
	duration_test		\
	parser_test

include $(top_srcdir)/Makefile.tests
