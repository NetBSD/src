include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=				\
	-DNAMED_PLUGINDIR=\"$(pkglibdir)\"

lib_LTLIBRARIES = libns.la

libns_ladir = $(includedir)/ns

libns_la_HEADERS =			\
	include/ns/client.h		\
	include/ns/events.h		\
	include/ns/hooks.h		\
	include/ns/interfacemgr.h	\
	include/ns/listenlist.h		\
	include/ns/log.h		\
	include/ns/notify.h		\
	include/ns/query.h		\
	include/ns/server.h		\
	include/ns/sortlist.h		\
	include/ns/stats.h		\
	include/ns/types.h		\
	include/ns/update.h		\
	include/ns/xfrout.h

libns_la_SOURCES =		\
	$(libns_la_HEADERS)	\
	client.c		\
	hooks.c			\
	interfacemgr.c		\
	listenlist.c		\
	log.c			\
	notify.c		\
	query.c			\
	server.c		\
	sortlist.c		\
	stats.c			\
	update.c		\
	xfrout.c

libns_la_CPPFLAGS =				\
	$(AM_CPPFLAGS)				\
	$(LIBDNS_CFLAGS)			\
	$(LIBISC_CFLAGS)			\
	$(LIBNS_CFLAGS)				\
	$(LIBUV_CFLAGS)				\
	$(OPENSSL_CFLAGS)

libns_la_LIBADD =	\
	$(LIBDNS_LIBS)	\
	$(LIBISC_LIBS)	\
	$(LIBUV_LIBS)	\
	$(OPENSSL_LIBS)

libns_la_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	-release "$(PACKAGE_VERSION)"
