include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)

noinst_LTLIBRARIES = sample.la

sample_la_SOURCES =	\
	db.c		\
	driver.c	\
	instance.c	\
	lock.c		\
	log.c		\
	syncptr.c	\
	zone.c		\
	db.h		\
	instance.h	\
	lock.h		\
	log.h		\
	syncptr.h	\
	util.h		\
	zone.h

sample_la_LDFLAGS = -avoid-version -module -shared -export-dynamic -rpath $(abs_builddir)
