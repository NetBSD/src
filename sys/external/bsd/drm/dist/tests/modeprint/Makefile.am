AM_CFLAGS = \
	-I$(top_srcdir)/shared-core \
	-I$(top_srcdir)/libdrm/intel/ \
	-I$(top_srcdir)/libdrm

noinst_PROGRAMS = \
	modeprint

modeprint_SOURCES = \
	modeprint.c
modeprint_LDADD = \
	$(top_builddir)/libdrm/libdrm.la \
	$(top_builddir)/libdrm/intel/libdrm_intel.la
