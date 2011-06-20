## Makefile.am -- Process this file with automake to produce Makefile.in
AM_CFLAGS=@WARNINGCFLAGS@
AM_CPPFLAGS = -I$(top_srcdir)/src
LDADD = libmpc-tests.la $(top_builddir)/src/libmpc.la
# LOADLIBES (documented in the "GNU make" manual and equivalent to LDLIBS)
# enables to compile a program foo.c in the test directory by simply doing
# "make foo".
LOADLIBES=$(DEFS) -I$(top_srcdir)/src -I$(top_builddir) $(CPPFLAGS) $(CFLAGS) $(top_builddir)/tests/.libs/libmpc-tests.a $(top_builddir)/src/.libs/libmpc.a $(LIBS)

check_PROGRAMS = tabs tacos tacosh tadd tadd_fr tadd_si tadd_ui targ tasin    \
tasinh tatan tatanh tconj tcos tcosh tdiv tdiv_2exp tdiv_fr tdiv_ui texp tfma \
tfr_div tfr_sub timag tio_str tlog tmul tmul_2exp tmul_fr tmul_i tmul_si      \
tmul_ui tneg tnorm tpow tpow_ld tpow_d tpow_fr tpow_si tpow_ui tpow_z tprec   \
tproj treal treimref tset tsin tsin_cos tsinh tsqr tsqrt tstrtoc tsub tsub_fr \
tsub_ui ttan ttanh tui_div tui_ui_sub tget_version

check_LTLIBRARIES=libmpc-tests.la
libmpc_tests_la_SOURCES=mpc-tests.h random.c tgeneric.c read_data.c           \
comparisons.c memory.c

EXTRA_DIST = abs.dat acos.dat acosh.dat asin.dat asinh.dat atan.dat atanh.dat \
add.dat add_fr.dat arg.dat conj.dat cos.dat cosh.dat div.dat div_fr.dat       \
exp.dat fma.dat fr_div.dat fr_sub.dat inp_str.dat log.dat mul.dat mul_fr.dat  \
neg.dat norm.dat pow.dat pow_ui.dat proj.dat sin.dat sinh.dat sqr.dat         \
sqrt.dat strtoc.dat sub.dat sub_fr.dat tan.dat tanh.dat

TESTS = $(check_PROGRAMS)
CLEANFILES = mpc_test

