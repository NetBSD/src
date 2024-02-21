include $(top_srcdir)/Makefile.top

lib_LTLIBRARIES = libirs.la

libirs_ladir = $(includedir)/irs
libirs_la_HEADERS =		\
	include/irs/resconf.h

libirs_la_SOURCES =		\
	$(libirs_la_HEADERS)	\
	resconf.c

libirs_la_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	$(LIBISCCFG_CFLAGS)	\
	$(LIBIRS_CFLAGS)

libirs_la_LIBADD =		\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)		\
	$(LIBISCCFG_LIBS)

libirs_la_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	-release "$(PACKAGE_VERSION)"
