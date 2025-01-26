include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)

LDADD +=			\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)

check_PROGRAMS =		\
	resconf_test

EXTRA_DIST = testdata

include $(top_srcdir)/Makefile.tests
