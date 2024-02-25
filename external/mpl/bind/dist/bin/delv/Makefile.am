include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=				\
	-I$(top_builddir)/include	\
	$(LIBISC_CFLAGS)		\
	$(LIBDNS_CFLAGS)		\
	$(LIBISCCFG_CFLAGS)		\
	$(LIBIRS_CFLAGS)

AM_CPPFLAGS +=				\
	-DSYSCONFDIR=\"${sysconfdir}\"

bin_PROGRAMS = delv

delv_SOURCES =				\
	delv.c
delv_LDADD =				\
	$(LIBISC_LIBS)			\
	$(LIBDNS_LIBS)			\
	$(LIBISCCFG_LIBS)		\
	$(LIBIRS_LIBS)
