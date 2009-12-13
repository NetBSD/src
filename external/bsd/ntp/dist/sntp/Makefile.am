# Makefile.am for JMK's SNTP, by Harlan Stenn

NULL=

AUTOMAKE_OPTIONS = foreign 1.10
ACLOCAL_AMFLAGS= -I ../m4 -I libopts/m4

AM_CPPFLAGS= $(LIBOPTS_CFLAGS) -I$(top_srcdir)/../include \
	-I$(top_srcdir)/../lib/isc/include \
	-I$(top_srcdir)/../lib/isc/nothreads/include \
	-I$(top_srcdir)/../lib/isc/unix/include

LDADD=	$(LIBOPTS_LDADD) -lm ../libntp/libntp.a

run_ag=	cd $(srcdir) &&				\
	env PATH="$(abs_builddir):$(PATH)"	\
	autogen -L ../include --writable

bindir=	${exec_prefix}/${BINSUBDIR}
bin_PROGRAMS=	sntp
CLEANFILES=	libopts-subdir

SUBDIRS=
if NEED_LIBOPTS
SUBDIRS+= libopts
endif
SUBDIRS+= .

sntp_SOURCES =		\
	crypto.c	\
	crypto.h	\
	data_formats.h	\
	header.h	\
	kod_management.c \
	kod_management.h \
	log.c		\
	log.h		\
	main.c		\
	networking.c	\
	networking.h	\
	sntp-opts.c	\
	sntp-opts.h	\
	utilities.c	\
	utilities.h	\
	$(NULL)

# it's in EXTRA_DIST
#dist_man_MANS=	sntp.1

EXTRA_DIST=				\
		autogen-version.def	\
		bincheck.mf		\
		COPYRIGHT		\
		deps-ver		\
		depsver.mf		\
		sntp-opts.def		\
		sntp-opts.menu		\
		sntp-opts.texi		\
		sntp.1			\
		sntp.html		\
		sntp.texi		\
		version.def		\
		version.m4		\
		version.texi		\
		$(NULL)

OLD_EXTRA_DIST=		\
		autogen-version.def version.def version.m4 version.texi

BUILT_SOURCES=						\
		check-autogen-version.def		\
		check-version.def check-version.m4	\
		check-version.texi			\
		libtool					\
		sntp-opts.c				\
		sntp-opts.h				\
		sntp-opts.menu				\
		sntp-opts.texi				\
		sntp.1					\
		sntp.html				\
		$(NULL)

man_MANS=	sntp.1

FRC:
check-autogen-version.def: FRC
	@cd $(srcdir)							\
	&& test -r ../include/autogen-version.def			\
	&& ( if cmp -s ../include/autogen-version.def autogen-version.def; \
	   then : ;							\
	   else cp ../include/autogen-version.def autogen-version.def;	 \
		echo "Installing new sntp/autogen-version.def file";	\
	   fi )

check-version.def: FRC
	@cd $(srcdir)					\
	&& test -r ../include/version.def		\
	&& ( if cmp -s ../include/version.def version.def;	\
	   then : ;						\
	   else cp ../include/version.def version.def;		\
		echo "Installing new sntp/version.def file";	\
	   fi )

check-version.m4: FRC
	@cd $(srcdir)					\
	&& test -r ../version.m4			\
	&& ( if cmp -s ../version.m4 version.m4;	\
	   then : ;					\
	   else cp ../version.m4 version.m4;		\
		echo "Installing new sntp/version.m4 file";	\
	   fi )

check-version.texi: FRC
	@cd $(srcdir)					\
	&& test -r ../include/version.texi			\
	&& ( if cmp -s ../include/version.texi version.texi;	\
	   then : ;					\
	   else cp ../include/version.texi version.texi;	\
		echo "Installing new sntp/version.texi file";	\
	   fi )

$(srcdir)/../COPYRIGHT:
	cd .. && $(MAKE) COPYRIGHT

$(srcdir)/COPYRIGHT: $(srcdir)/../COPYRIGHT
	cat $(srcdir)/../COPYRIGHT > $@

$(srcdir)/sntp-opts.h: $(srcdir)/sntp-opts.c
	@: do-nothing action to avoid default SCCS get, .h built with .c
	
$(srcdir)/sntp-opts.c: $(srcdir)/sntp-opts.def $(srcdir)/../include/version.def
	$(run_ag) sntp-opts.def

$(srcdir)/sntp.1: $(srcdir)/sntp-opts.def $(srcdir)/../include/version.def
	$(run_ag) -Tagman1.tpl -bsntp sntp-opts.def

$(srcdir)/sntp-opts.texi $(srcdir)/sntp-opts.menu: $(srcdir)/sntp-opts.def $(srcdir)/../include/version.def
	$(MAKE) libopts-subdir sntp	# aginfo.tpl runs binary to extract --help usage text
	$(run_ag) -Taginfo.tpl -DLEVEL=section sntp-opts.def

libopts-subdir: $(LIBOPTS_LDADD)
	touch $@	# LIBOPTS_LDADD is ./libopts/libopts.la if NEED_LIBOPTS

libopts/libopts.la:
	( cd libopts && $(MAKE) libopts.la )

$(srcdir)/sntp.html: $(srcdir)/sntp-opts.menu $(srcdir)/sntp-opts.texi $(srcdir)/sntp.texi $(srcdir)/version.texi
	cd $(srcdir) && makeinfo --force --html --no-split -o sntp.html sntp.texi

../libntp/libntp.a:
	cd ../libntp && $(MAKE)

libtool: $(LIBTOOL_DEPS)
	./config.status --recheck

include $(top_srcdir)/bincheck.mf
include $(top_srcdir)/depsver.mf
