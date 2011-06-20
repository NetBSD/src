# Copyright 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
# This Makefile.am is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


AUTOMAKE_OPTIONS = 1.6 gnu $(top_builddir)/ansi2knr

check_PROGRAMS = tversion tinternals tinits tisqrt tsgn tcheck	\
     tisnan texceptions tset_exp tset mpf_compat mpfr_compat	\
     reuse tabs tacos tacosh tadd tadd1sp tadd_d tadd_ui tagm	\
     tasin tasinh tatan tatanh taway tbuildopt tcan_round	\
     tcbrt tcmp tcmp2 tcmp_d tcmp_ld tcmp_ui tcmpabs		\
     tcomparisons tconst_catalan tconst_euler tconst_log2	\
     tconst_pi tcopysign tcos tcosh tcot tcoth tcsc tcsch	\
     td_div td_sub tdigamma tdim tdiv tdiv_d tdiv_ui teint teq	\
     terf texp texp10 texp2 texpm1 tfactorial tfits tfma tfmod	\
     tfms tfprintf tfrac tgamma tget_flt tget_d tget_d_2exp	\
     tget_f tget_ld_2exp tget_set_d64 tget_sj tget_str tget_z	\
     tgmpop thyperbolic thypot tinp_str tj0 tj1 tjn tl2b	\
     tlgamma tli2 tlngamma tlog tlog10 tlog1p tlog2 tmin_prec	\
     tminmax tmodf tmul tmul_2exp tmul_d tmul_ui tnext		\
     tout_str toutimpl tpow tpow3 tpow_all tpow_z tprintf	\
     trandom trec_sqrt tremquo trint troot tround_prec tsec	\
     tsech tset_d tset_f tset_ld tset_q tset_si tset_sj		\
     tset_str tset_z tset_z_exp tsi_op tsin tsin_cos tsinh      \
     tsinh_cosh tsprintf tsqr tsqrt tsqrt_ui tstckintc tstrtofr \
     tsub tsub1sp tsub_d tsub_ui tsubnormal tsum tswap turandom \
     ttan ttanh ttrunc tui_div tui_pow tui_sub ty0 ty1 tyn	\
     tzeta tzeta_ui tai

EXTRA_DIST = tgeneric.c tgeneric_ui.c mpf_compat.h inp_str.data tmul.dat

LDADD = libfrtests.la $(MPFR_LIBM) $(top_builddir)/libmpfr.la
INCLUDES = -I$(top_srcdir) -I$(top_builddir)

# LOADLIBES (documented in the "GNU make" manual and equivalent to LDLIBS)
# enables to compile a program foo.c in the test directory by simply doing
# "make foo".
LOADLIBES=$(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(top_builddir)/tests/.libs/libfrtests.a $(top_builddir)/.libs/libmpfr.a $(LIBS) $(MPFR_LIBM)

check_LTLIBRARIES = libfrtests.la
libfrtests_la_SOURCES = mpfr-test.h memory.c rnd_mode.c tests.c cmp_str.c random2.c

$(top_builddir)/libmpfr.la:
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) libmpfr.la

TESTS = $(check_PROGRAMS)
TESTS_ENVIRONMENT = MPFR_QUIET=1
