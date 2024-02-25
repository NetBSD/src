include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBIRS_CFLAGS)

LDADD +=			\
	$(LIBISC_LIBS)		\
	$(LIBIRS_LIBS)

check_PROGRAMS =		\
	resconf_test

EXTRA_DIST = testdata

include $(top_srcdir)/Makefile.tests
