NULL=
AUTOMAKE_OPTIONS= 

sbin_PROGRAMS=	@MAKE_NTPSNMPD@
EXTRA_PROGRAMS=	ntpsnmpd
ntpsnmpd_SOURCES= netsnmp_daemonize.c ntpsnmpd.c ntpSnmpSubagentObject.c \
			ntpsnmpd-opts.c ntpsnmpd-opts.h
noinst_HEADERS=	ntpSnmpSubagentObject.h
# HMS: we probably want a version.o file here, too.
LDADD=		../ntpq/libntpq.a ../libntp/libntp.a @SNMP_LIBS@ 	\
		$(LIBOPTS_LDADD)
AM_CPPFLAGS= -I$(top_srcdir)/ntpq -I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/isc/include \
	-I$(top_srcdir)/lib/isc/nothreads/include \
	-I$(top_srcdir)/lib/isc/unix/include \
	$(LIBOPTS_CFLAGS) @SNMP_CPPFLAGS@

AM_CFLAGS=	@SNMP_CFLAGS@
EXTRA_DIST=	ntpsnmpd-opts.def ntpsnmpd.1 ntpsnmpd-opts.texi	\
		ntpsnmpd-opts.menu ntpv4-mib.mib
BUILT_SOURCES=	ntpsnmpd-opts.c ntpsnmpd-opts.h ntpsnmpd.1 ntpsnmpd-opts.texi \
		ntpsnmpd-opts.menu
CLEANFILES=
man_MANS=	ntpsnmpd.1
run_ag=		cd $(srcdir) &&					\
		env PATH="$(abs_builddir):$(PATH)"		\
		autogen -L ../include --writable
std_def_list=	$(top_srcdir)/include/autogen-version.def 	\
		$(top_srcdir)/include/copyright.def 		\
		$(top_srcdir)/include/homerc.def 		\
		$(top_srcdir)/include/version.def

$(srcdir)/ntpsnmpd-opts.h: $(srcdir)/ntpsnmpd-opts.c
	@: do-nothing action to avoid default SCCS get, .h built with .c
	
$(srcdir)/ntpsnmpd-opts.c: $(srcdir)/ntpsnmpd-opts.def $(std_def_list)
	$(run_ag) ntpsnmpd-opts.def

$(srcdir)/ntpsnmpd.1: $(srcdir)/ntpsnmpd-opts.def $(std_def_list)
	$(run_ag) -Tagman1.tpl -bntpsnmpd ntpsnmpd-opts.def

$(srcdir)/ntpsnmpd-opts.texi $(srcdir)/ntpsnmpd-opts.menu: $(srcdir)/ntpsnmpd-opts.def $(std_def_list)
	$(MAKE) @MAKE_NTPSNMPD@ not-all-target	# aginfo.tpl runs the binary to extract --help usage text
	$(run_ag) -Taginfo.tpl -DLEVEL=section ntpsnmpd-opts.def

not-all-target:
	touch $@	# used to avoid "all" default target when not building ntpsnmpd

include $(top_srcdir)/bincheck.mf
include $(top_srcdir)/depsver.mf
