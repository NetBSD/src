include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	$(LIBISCCFG_CFLAGS)	\
	$(LIBIRS_CFLAGS)	\
	$(LIBBIND9_CFLAGS)	\
	$(LIBIDN2_CFLAGS)

LDADD +=			\
	libdighost.la		\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)		\
	$(LIBISCCFG_LIBS)	\
	$(LIBIRS_LIBS)		\
	$(LIBBIND9_LIBS)	\
	$(LIBIDN2_LIBS)

noinst_LTLIBRARIES = libdighost.la

libdighost_la_SOURCES =		\
	dighost.h		\
	dighost.c

bin_PROGRAMS = dig host nslookup

nslookup_CPPFLAGS =		\
	$(AM_CPPFLAGS)

nslookup_LDADD =		\
	$(LDADD)

if HAVE_READLINE
nslookup_CPPFLAGS +=		\
	$(READLINE_CFLAGS)
nslookup_LDADD +=		\
	$(READLINE_LIBS)
endif HAVE_READLINE
