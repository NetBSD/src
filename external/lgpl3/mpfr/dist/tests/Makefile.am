# Copyright 2000-2018 Free Software Foundation, Inc.
# This Makefile.am is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


AUTOMAKE_OPTIONS = 1.6 gnu

check_PROGRAMS = tversion tabort_prec_max tassert tabort_defalloc1	\
     tabort_defalloc2 talloc tinternals tinits tisqrt tsgn tcheck	\
     tisnan texceptions tset_exp tset mpf_compat mpfr_compat reuse	\
     tabs tacos tacosh tadd tadd1sp tadd_d tadd_ui tagm tai		\
     talloc-cache tasin tasinh tatan tatanh taway tbeta tbuildopt	\
     tcan_round tcbrt tcmp tcmp2 tcmp_d tcmp_ld tcmp_ui tcmpabs		\
     tcomparisons tconst_catalan tconst_euler tconst_log2 tconst_pi	\
     tcopysign tcos tcosh tcot tcoth tcsc tcsch td_div td_sub tdigamma	\
     tdim tdiv tdiv_d tdiv_ui teint teq terandom terandom_chisq terf	\
     texp texp10 texp2 texpm1 tfactorial tfits tfma tfmma tfmod tfms	\
     tfpif tfprintf tfrac tfrexp tgamma tgamma_inc tget_d tget_d_2exp	\
     tget_f tget_flt tget_ld_2exp tget_q tget_set_d64 tget_sj		\
     tget_str tget_z tgmpop tgrandom thyperbolic thypot tinp_str	\
     tj0 tj1 tjn tl2b tlgamma tli2 tlngamma tlog tlog10 tlog1p tlog2	\
     tlog_ui tmin_prec tminmax tmodf tmul tmul_2exp tmul_d tmul_ui	\
     tnext tnrandom tnrandom_chisq tout_str toutimpl tpow tpow3		\
     tpow_all tpow_z tprintf trandom trandom_deviate trec_sqrt		\
     tremquo trint trndna troot trootn_ui tround_prec tsec tsech	\
     tset_d tset_f tset_float128 tset_ld tset_q tset_si tset_sj		\
     tset_str tset_z tset_z_exp tsi_op tsin tsin_cos tsinh tsinh_cosh	\
     tsprintf tsqr tsqrt tsqrt_ui tstckintc tstdint tstrtofr		\
     tsub tsub1sp tsub_d tsub_ui tsubnormal tsum tswap ttan ttanh	\
     ttrunc tui_div tui_pow tui_sub turandom tvalist ty0 ty1 tyn	\
     tzeta tzeta_ui

# Before Automake 1.13, we ran tversion at the beginning and at the end
# of the tests, and output from tversion appeared at the same place as
# the tests results (make output). With Automake 1.13+, the tests are
# parallelized by default and their output is sent to log files instead
# of the make output, so that the user could no longer see information
# from tversion. To mimic the old behavior, we now output the contents
# of the tversion log file if this file exists (i.e. if the Makefile
# has been generated with Automake 1.13+); but this is done only when
# no tests fail.
check:
	cat tversion.log 2> /dev/null || true

AM_CPPFLAGS = -DSRCDIR='"$(srcdir)"'

EXTRA_DIST = tgeneric.c tgeneric_ui.c mpf_compat.h inp_str.dat tmul.dat \
	tfpif_r1.dat tfpif_r2.dat

CLEANFILES = tfpif_rw.dat tfprintf_out.txt tout_str_out.txt toutimpl_out.txt tprintf_out.txt

LDADD = libfrtests.la $(MPFR_LIBM) $(MPFR_LIBQUADMATH) $(top_builddir)/src/libmpfr.la
AM_CPPFLAGS += -I$(top_srcdir)/src -I$(top_builddir)/src

# LOADLIBES (documented in the "GNU make" manual and equivalent to LDLIBS)
# enables to compile a program foo.c in the test directory by simply doing
# "make foo".
# Warning! This is not guaranteed to work, as libtool is not used. In
# particular, this may not work as expected under GNU/Linux if --with-gmp
# has been used, unless the directory is in your $LD_LIBRARY_PATH.
# Moreover, dependencies are not tracked. Thus you may want to run
# "make tversion" (for instance) just before, to make sure that every
# dependency has been rebuilt.
LOADLIBES=$(DEFS) $(AM_CPPFLAGS) $(CPPFLAGS) $(CFLAGS) $(top_builddir)/tests/.libs/libfrtests.a $(top_builddir)/src/.libs/libmpfr.a $(LIBS) $(MPFR_LIBM)

check_LTLIBRARIES = libfrtests.la
libfrtests_la_SOURCES = mpfr-test.h memory.c rnd_mode.c tests.c cmp_str.c random2.c

$(top_builddir)/src/libmpfr.la:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) libmpfr.la

TESTS = $(check_PROGRAMS)

# $(VALGRIND) is still there to be able to use a wrapper such as valgrind
# in case old Automake. With Automake 1.13+, where a parallel test harness
# is generated, use LOG_COMPILER instead. For additional information, see:
# http://www.gnu.org/software/automake/manual/automake.html#Parallel-Test-Harness
TESTS_ENVIRONMENT = MPFR_QUIET=1 $(VALGRIND)

# The -no-install option prevents libtool from generating wrapper scripts
# for the tests.
# This is useful to easily run the test scripts under valgrind or gdb.
# See discussion http://thread.gmane.org/gmane.comp.lib.gnulib.bugs/28033
# http://article.gmane.org/gmane.comp.lib.gnulib.bugs/28140 in particular.
#
# The -L$(top_builddir)/src/.libs option is necessary for some platforms,
# such as HP-UX, when --with-gmp or --with-gmp-lib is used and an old MPFR
# library is already installed in the corresponding lib directory: its
# purpose is to make sure that the local .libs comes first in the library
# search path (otherwise the tests are linked against the old MPFR library
# by the LINK command -- see the generated Makefile). See:
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00042.html
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00043.html
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00044.html
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00066.html
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00065.html
# and
#   http://debbugs.gnu.org/cgi/bugreport.cgi?bug=9728
#   http://debbugs.gnu.org/cgi/bugreport.cgi?bug=18662
#
AM_LDFLAGS = -no-install -L$(top_builddir)/src/.libs
