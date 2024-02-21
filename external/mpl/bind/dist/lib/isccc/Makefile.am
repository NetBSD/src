include $(top_srcdir)/Makefile.top

lib_LTLIBRARIES = libisccc.la

libisccc_ladir = $(includedir)/isccc
libisccc_la_HEADERS =			\
	include/isccc/alist.h		\
	include/isccc/base64.h		\
	include/isccc/cc.h		\
	include/isccc/ccmsg.h		\
	include/isccc/events.h		\
	include/isccc/sexpr.h		\
	include/isccc/symtab.h		\
	include/isccc/symtype.h		\
	include/isccc/types.h		\
	include/isccc/util.h

libisccc_la_SOURCES =		\
	$(libisccc_la_HEADERS)	\
	alist.c			\
	base64.c		\
	cc.c			\
	ccmsg.c			\
	sexpr.c			\
	symtab.c

libisccc_la_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)	\
	$(LIBISCCC_CFLAGS)

libisccc_la_LIBADD =	\
	$(LIBISC_LIBS)

libisccc_la_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	-release "$(PACKAGE_VERSION)"
