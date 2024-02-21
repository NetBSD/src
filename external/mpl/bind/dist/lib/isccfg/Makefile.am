include $(top_srcdir)/Makefile.top

lib_LTLIBRARIES = libisccfg.la

libisccfg_ladir = $(includedir)/isccfg
libisccfg_la_HEADERS =			\
	include/isccfg/aclconf.h	\
	include/isccfg/cfg.h		\
	include/isccfg/duration.h	\
	include/isccfg/grammar.h	\
	include/isccfg/kaspconf.h	\
	include/isccfg/log.h		\
	include/isccfg/namedconf.h

libisccfg_la_SOURCES =			\
	$(libisccfg_la_HEADERS)		\
	aclconf.c			\
	dnsconf.c			\
	duration.c			\
	kaspconf.c			\
	log.c				\
	namedconf.c			\
	parser.c

libisccfg_la_CPPFLAGS =			\
	$(AM_CPPFLAGS)			\
	$(LIBISCCFG_CFLAGS)		\
	$(LIBISC_CFLAGS)		\
	$(LIBDNS_CFLAGS)

libisccfg_la_LIBADD =	\
	$(LIBDNS_LIBS)	\
	$(LIBISC_LIBS)

libisccfg_la_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	-release "$(PACKAGE_VERSION)"
