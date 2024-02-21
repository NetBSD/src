include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	$(LIBISCCFG_CFLAGS)	\
	$(LIBISCCC_CFLAGS)	\
	$(LIBBIND9_CFLAGS)

AM_CPPFLAGS +=						\
	-DRNDC_CONFFILE=\"${sysconfdir}/rndc.conf\"	\
	-DRNDC_KEYFILE=\"${sysconfdir}/rndc.key\"

sbin_PROGRAMS = rndc

rndc_SOURCES =	\
	rndc.c	\
	util.c	\
	util.h

rndc_LDADD =			\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)		\
	$(LIBISCCC_LIBS)	\
	$(LIBISCCFG_LIBS)	\
	$(LIBBIND9_LIBS)
