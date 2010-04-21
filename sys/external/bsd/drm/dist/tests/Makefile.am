AM_CPPFLAGS = \
	-I $(top_srcdir)/shared-core \
	-I $(top_srcdir)/libdrm

LDADD = $(top_builddir)/libdrm/libdrm.la

noinst_PROGRAMS = \
	dristat \
	drmstat

SUBDIRS = \
	modeprint \
	modetest

if HAVE_LIBUDEV

EXTRA_LTLIBRARIES = libdrmtest.la

libdrmtest_la_SOURCES = \
	drmtest.c \
	drmtest.h

libdrmtest_la_LIBADD = \
	$(top_builddir)/libdrm/libdrm.la \
	$(LIBUDEV_LIBS)

LDADD += libdrmtest.la

XFAIL_TESTS =					\
	auth					\
	lock

TESTS =						\
	openclose				\
	getversion				\
	getclient				\
	getstats				\
	setversion				\
	updatedraw				\
	gem_basic				\
	gem_flink				\
	gem_readwrite				\
	gem_mmap

EXTRA_PROGRAMS = $(TESTS)

endif

CLEANFILES = $(EXTRA_PROGRAMS) $(EXTRA_LTLIBRARIES)

