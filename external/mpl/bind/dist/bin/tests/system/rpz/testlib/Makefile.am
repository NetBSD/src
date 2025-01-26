include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)

AM_CFLAGS += -Wall -pedantic

noinst_LTLIBRARIES = libdummyrpz.la
libdummyrpz_la_SOURCES= dummylib.c test-data.c trpz.h test-data.h
libdummyrpz_la_LDFLAGS = -avoid-version -module -shared -export-dynamic -rpath $(abs_builddir)
LDADD += -lpthread $(DLOPEN_LIBS)
