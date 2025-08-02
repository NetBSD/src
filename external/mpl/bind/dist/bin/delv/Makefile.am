include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=				\
	-I$(top_builddir)/include	\
	$(LIBISC_CFLAGS)		\
	$(LIBDNS_CFLAGS)		\
	$(LIBNS_CFLAGS)			\
	$(LIBISCCFG_CFLAGS)		\
	$(OPENSSL_CFLAGS)

AM_CPPFLAGS +=				\
	-DSYSCONFDIR=\"${sysconfdir}\"

bin_PROGRAMS = delv

delv_SOURCES =				\
	delv.c

delv_LDADD =				\
	$(LIBISC_LIBS)			\
	$(LIBDNS_LIBS)			\
	$(LIBNS_LIBS)			\
	$(LIBISCCFG_LIBS)
