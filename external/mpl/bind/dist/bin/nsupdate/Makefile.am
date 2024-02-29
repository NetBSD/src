include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	$(LIBISCCFG_CFLAGS)	\
	$(LIBIRS_CFLAGS)	\
	$(LIBBIND9_CFLAGS)	\
	$(GSSAPI_CFLAGS)	\
	$(KRB5_CFLAGS)		\
	$(READLINE_CFLAGS)

AM_CPPFLAGS +=			\
	-DSESSION_KEYFILE=\"${localstatedir}/run/named/session.key\"

LDADD +=			\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)		\
	$(LIBISCCFG_LIBS)	\
	$(LIBIRS_LIBS)		\
	$(LIBBIND9_LIBS)	\
	$(GSSAPI_LIBS)		\
	$(KRB5_LIBS)

if HAVE_READLINE
LDADD +=			\
	$(READLINE_LIBS)
endif

bin_PROGRAMS = nsupdate

nsupdate_SOURCES =		\
	nsupdate.c		\
	../dig/readline.h
