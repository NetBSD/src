include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	$(LIBNS_CFLAGS)		\
	$(LIBISCCFG_CFLAGS)

pkglib_LTLIBRARIES = filter-aaaa.la
pkglib_LTLIBRARIES += filter-a.la

filter_aaaa_la_SOURCES = filter-aaaa.c
filter_a_la_SOURCES = filter-a.c
filter_aaaa_la_LDFLAGS = -avoid-version -module -shared -export-dynamic
filter_a_la_LDFLAGS = -avoid-version -module -shared -export-dynamic
