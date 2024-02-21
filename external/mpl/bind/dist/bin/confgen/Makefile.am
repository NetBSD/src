include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	-DRNDC_KEYFILE=\"${sysconfdir}/rndc.key\"

LDADD +=			\
	libconfgen.la		\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)

noinst_LTLIBRARIES = libconfgen.la

libconfgen_la_SOURCES =		\
	include/confgen/os.h	\
	keygen.c		\
	keygen.h		\
	os.c			\
	util.c			\
	util.h

sbin_PROGRAMS = tsig-keygen rndc-confgen

install-exec-hook:
	ln -f $(DESTDIR)$(sbindir)/tsig-keygen \
	      $(DESTDIR)$(sbindir)/ddns-confgen

uninstall-hook:
	-rm -f $(DESTDIR)$(sbindir)/ddns-confgen
