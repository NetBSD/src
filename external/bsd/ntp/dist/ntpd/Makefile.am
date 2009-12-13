NULL=
AUTOMAKE_OPTIONS= 

bindir=	${exec_prefix}/${BINSUBDIR}

bin_PROGRAMS=	ntpd @MAKE_NTPDSIM@

noinst_LIBRARIES=	libntpd.a

AM_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/lib/isc/include \
	-I$(top_srcdir)/lib/isc/nothreads/include \
	-I$(top_srcdir)/lib/isc/unix/include $(LIBOPTS_CFLAGS)

# LDADD might need RESLIB and ADJLIB.
LDADD=	version.o libntpd.a @LIBPARSE@
AM_YFLAGS=	-d -t -r all

if SAVECONFIG_ENABLED
if NTP_CROSSCOMPILE
CHECK_SAVECONFIG=
else
CHECK_SAVECONFIG= check-saveconfig
endif
else !SAVECONFIG_ENABLED
CHECK_SAVECONFIG=
endif

if NTP_CROSSCOMPILE
NTP_KEYWORD_OUT=ntp_keyword.dummy
else
NTP_KEYWORD_OUT=ntp_keyword.out
endif

#
# VPHACK and VPHACK_AFTER are enabled on non-GNU makes (such as 
# BSD make) to work around issues specific to compiling
# ntp_parser.y into ntp_parser.h and ntp_parser.c in a vPATH
# configuration where we would like (for a change) the output
# files ntp_parser.[ch] to be placed in the source directory,
# as opposed to the build directory.  This allows a single
# host of a flock configured with Bison to update ntp_parser.[ch]
# used by the rest.
# 

if VPATH_HACK
VPHACK=	vphack
VPHACK_AFTER= vphack_after
else
VPHACK=
VPHACK_AFTER=
endif

vphack:
	test -e ntp_parser.c || ln -s $(srcdir)/ntp_parser.c .
	test -e ntp_parser.h || ln -s $(srcdir)/ntp_parser.h .

#
# ylwrap script which invokes Bison replaces ntp_parser.h
# symlink with the updated file, when ntp_parser.h changes.
# vphack_after detects this and copies the updated file to srcdir 
# and re-creates the ntp_parser.h symlink in its place.
#

vphack_after:
	test -L ntp_parser.h || ( \
		mv ntp_parser.h $(srcdir)/ntp_parser.h && \
		ln -s $(srcdir)/ntp_parser.h . \
		)

BUILT_SOURCES=	$(VPHACK) ntp_parser.c ntp_parser.h $(VPHACK_AFTER) \
	ntp_keyword.h ntpd-opts.c ntpd-opts.h ntpd.1 ntpd-opts.texi \
	ntpd-opts.menu

man_MANS=	ntpd.1

# ntpdsim.1 is a remnant along with all the ntpdsim-opts.* files, the
# simulator currently uses ntpd-opts.[ch].  This also means there is no
# longer a reason to have ntpdbase-opts.def split off of ntpd-opts.def.

# ntpd may need:
# log10                               refclock_wwv.o
# sqrt                                ntp_control.o
# floor                               refclock_wwv.o
# which are (usually) provided by -lm.
keyword_gen_LDADD = ../libntp/libntp.a
ntpd_LDADD = $(LDADD) -lm @LCRYPTO@ @LSCF@ $(LIBOPTS_LDADD) ../libntp/libntp.a
ntpdsim_LDADD = $(LDADD) -lm @LCRYPTO@ @LSCF@ $(LIBOPTS_LDADD) ../libntp/libntpsim.a
ntpdsim_CFLAGS = $(CFLAGS) -DSIM
check_y2k_LDADD = $(LDADD) ../libntp/libntp.a
DISTCLEANFILES = keyword-gen .version version.c
CLEANFILES = check-saveconfig compsave.conf ntp_keyword.out \
	ntp_keyword.dummy
EXTRA_DIST = \
	complete.conf		\
	ntpd-opts.def		\
	ntpdbase-opts.def	\
	ntp_keyword.h		\
	refclock_msfees.c	\
	refclock_trak.c		\
	$(BUILT_SOURCES)	\
	$(NULL)
ETAGS_ARGS = Makefile.am
###							Y2Kfixes
check_PROGRAMS = @MAKE_CHECK_Y2K@
EXTRA_PROGRAMS = check_y2k ntpdsim keyword-gen
run_ag=		cd $(srcdir) &&					\
		env PATH="$(abs_builddir):$(PATH)"		\
		autogen -L ../include --writable
std_def_list=	$(top_srcdir)/include/autogen-version.def 	\
		$(top_srcdir)/include/copyright.def 		\
		$(top_srcdir)/include/version.def

check-local: @MAKE_CHECK_Y2K@ $(CHECK_SAVECONFIG)
	test -z "@MAKE_CHECK_Y2K@" || ./@MAKE_CHECK_Y2K@

ntpd_SOURCES =			\
	cmd_args.c		\
	ntp_config.c		\
	ntp_data_structures.c	\
	ntp_io.c		\
	ntp_parser.y		\
	ntp_scanner.c		\
	ntp_scanner.h		\
	ntpd.c			\
	ntpd-opts.c		\
	ntpd-opts.h		\
	$(NULL)
	
ntpdsim_SOURCES =		\
	$(ntpd_SOURCES)		\
	ntpsim.c		\
	$(NULL)

# libntpd_a_SOURCES do not use #ifdef SIM

libntpd_a_SOURCES =		\
	jupiter.h		\
	ntp_control.c		\
	ntp_crypto.c		\
	ntp_filegen.c		\
	ntp_intres.c		\
	ntp_loopfilter.c	\
	ntp_monitor.c		\
	ntp_peer.c		\
	ntp_proto.c		\
	ntp_refclock.c		\
	ntp_request.c		\
	ntp_restrict.c		\
	ntp_signd.c		\
	ntp_timer.c		\
	ntp_util.c		\
	ppsapi_timepps.h	\
	refclock_acts.c		\
	refclock_arbiter.c	\
	refclock_arc.c		\
	refclock_as2201.c	\
	refclock_atom.c		\
	refclock_bancomm.c	\
	refclock_chronolog.c	\
	refclock_chu.c		\
	refclock_conf.c		\
	refclock_datum.c	\
	refclock_dumbclock.c	\
	refclock_fg.c		\
	refclock_gpsvme.c	\
	refclock_heath.c	\
	refclock_hopfser.c	\
	refclock_hopfpci.c	\
	refclock_hpgps.c	\
	refclock_irig.c		\
	refclock_jjy.c		\
	refclock_jupiter.c	\
	refclock_leitch.c	\
	refclock_local.c	\
	refclock_mx4200.c	\
	refclock_neoclock4x.c	\
	refclock_nmea.c		\
	refclock_oncore.c	\
	refclock_palisade.c	\
	refclock_palisade.h	\
	refclock_parse.c	\
	refclock_pcf.c		\
	refclock_pst.c		\
	refclock_ripencc.c	\
	refclock_shm.c		\
	refclock_tpro.c		\
	refclock_true.c		\
	refclock_tt560.c	\
	refclock_ulink.c	\
	refclock_wwv.c		\
	refclock_wwvb.c		\
	refclock_zyfer.c	\
	$(NULL)

ntp_keyword.out: keyword-gen
	./keyword-gen $(srcdir)/ntp_parser.h > $@
	grep -v diff_ignore_line < $(srcdir)/ntp_keyword.h > cmp1
	grep -v diff_ignore_line < $@ > cmp2
	cmp cmp1 cmp2 > /dev/null || cp $@ $(srcdir)/ntp_keyword.h
	rm cmp1 cmp2
	
ntp_keyword.dummy:
	echo stamp > $@

$(srcdir)/ntp_keyword.h: $(NTP_KEYWORD_OUT)
	@: do-nothing action to avoid default SCCS get
	@: .h updated if needed by ntp_keyword.out rule

$(srcdir)/ntpd-opts.h: $(srcdir)/ntpd-opts.c
	@: do-nothing action to avoid default SCCS get, .h built with .c
	
$(srcdir)/ntpd-opts.c: $(srcdir)/ntpd-opts.def $(srcdir)/ntpdbase-opts.def $(std_def_list)
	$(run_ag) ntpd-opts.def

$(srcdir)/ntpd.1: $(srcdir)/ntpd-opts.def $(srcdir)/ntpdbase-opts.def $(std_def_list)
	$(run_ag) -Tagman1.tpl -bntpd ntpd-opts.def

$(srcdir)/ntpd-opts.texi $(srcdir)/ntpd-opts.menu: $(srcdir)/ntpd-opts.def $(srcdir)/ntpdbase-opts.def $(std_def_list)
	$(MAKE) ntpd	# aginfo.tpl runs the binary to extract --help usage text
	$(run_ag) -Taginfo.tpl -DLEVEL=section ntpd-opts.def

$(PROGRAMS): $(LDADD)

compsave.conf: ntpd $(srcdir)/complete.conf
	./ntpd --configfile $(srcdir)/complete.conf --saveconfigquit $@

check-saveconfig: $(srcdir)/complete.conf compsave.conf
	cmp $(srcdir)/complete.conf compsave.conf && echo stamp > $@

../libntp/libntp.a:
	cd ../libntp && $(MAKE) libntp.a

../libparse/libparse.a:
	cd ../libparse && $(MAKE)

$(top_srcdir)/version :
	cd $(top_srcdir) && $(MAKE) version

version.o: $(ntpd_OBJECTS) ../libntp/libntp.a @LIBPARSE@ Makefile $(top_srcdir)/version
	env CSET=`cat $(top_srcdir)/version` $(top_builddir)/scripts/mkver ntpd
	$(COMPILE) -c version.c

include $(top_srcdir)/bincheck.mf
include $(top_srcdir)/depsver.mf
