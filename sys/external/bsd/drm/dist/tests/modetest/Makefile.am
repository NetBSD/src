AM_CFLAGS = \
	-I$(top_srcdir)/shared-core \
	-I$(top_srcdir)/libdrm/intel/ \
	-I$(top_srcdir)/libdrm \
	$(CAIRO_CFLAGS)

noinst_PROGRAMS = \
	modetest

modetest_SOURCES = \
	modetest.c
modetest_LDADD = \
	$(top_builddir)/libdrm/libdrm.la \
	$(top_builddir)/libdrm/intel/libdrm_intel.la \
	$(CAIRO_LIBS)
