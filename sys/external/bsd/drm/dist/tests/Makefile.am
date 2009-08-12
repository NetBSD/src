AM_CFLAGS = \
	-I $(top_srcdir)/shared-core \
	-I $(top_srcdir)/libdrm

noinst_PROGRAMS = \
	dristat \
	drmstat

EXTRA_LTLIBRARIES = libdrmtest.la
libdrmtest_la_SOURCES = \
	drmtest.c \
	drmtest.h
libdrmtest_la_LIBADD = \
	$(top_builddir)/libdrm/libdrm.la

LDADD = libdrmtest.la

TESTS = auth \
	openclose \
	getversion \
	getclient \
	getstats \
	lock \
	setversion \
	updatedraw

EXTRA_PROGRAMS = $(TESTS)
CLEANFILES = $(EXTRA_PROGRAMS) $(EXTRA_LTLIBRARIES)

