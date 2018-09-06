## tests/Makefile.am -- Process this file with automake to produce Makefile.in
##
## Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2016 INRIA
##
## This file is part of GNU MPC.
##
## GNU MPC is free software; you can redistribute it and/or modify it under
## the terms of the GNU Lesser General Public License as published by the
## Free Software Foundation; either version 3 of the License, or (at your
## option) any later version.
##
## GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
## FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
## more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this program. If not, see http://www.gnu.org/licenses/ .

AM_CPPFLAGS = -I$(top_srcdir)/src
LDADD = libmpc-tests.la $(top_builddir)/src/libmpc.la
# let libtool create an executable instead of a shell script
# useful for tests with valgrind
AM_LDFLAGS = -no-install
# LOADLIBES (documented in the "GNU make" manual and equivalent to LDLIBS)
# enables to compile a program foo.c in the test directory by simply doing
# "make foo".
LOADLIBES=$(DEFS) $(AM_CPPFLAGS) $(CPPFLAGS) $(CFLAGS)	\
  $(top_builddir)/tests/.libs/libmpc-tests.a		\
  $(top_builddir)/src/.libs/libmpc.a $(LIBS)

check_PROGRAMS = tabs tacos tacosh tadd tadd_fr tadd_si tadd_ui targ	\
  tasin tasinh tatan tatanh tcmp_abs tconj tcos tcosh                   \
  tdiv tdiv_2si tdiv_2ui	                                        \
  tdiv_fr tdiv_ui texp tfma tfr_div tfr_sub timag tio_str tlog tlog10	\
  tmul tmul_2si tmul_2ui tmul_fr tmul_i tmul_si tmul_ui tneg tnorm tpow	\
  tpow_d tpow_fr tpow_ld tpow_si tpow_ui tpow_z tprec tproj treal	\
  treimref trootofunity                                                 \
  tset tsin tsin_cos tsinh tsqr tsqrt tstrtoc tsub tsub_fr	        \
  tsub_ui tswap ttan ttanh tui_div tui_ui_sub tget_version exceptions

check_LTLIBRARIES=libmpc-tests.la
libmpc_tests_la_SOURCES = mpc-tests.h check_data.c clear_parameters.c	\
  close_datafile.c comparisons.c copy_parameter.c double_rounding.c	\
  init_parameters.c mpfr_flags.c open_datafile.c print_parameter.c	\
  random.c read_data.c read_description.c read_line.c rounding.c	\
  setprec_parameters.c tpl_gmp.c tpl_mpc.c tpl_mpfr.c tpl_native.c

DESCRIPTIONS = abs.dsc acos.dsc acosh.dsc add.dsc add_fr.dsc add_si.dsc	\
  add_ui.dsc arg.dsc asin.dsc asinh.dsc atan.dsc atanh.dsc              \
  cmp_abs.dsc conj.dsc	\
  cos.dsc cosh.dsc div.dsc div_2si.dsc div_2ui.dsc div_fr.dsc           \
  div_ui.dsc exp.dsc fma.dsc fr_div.dsc fr_sub.dsc imag.dsc log.dsc	\
  log10.dsc mul.dsc mul_2si.dsc mul_2ui.dsc mul_fr.dsc mul_i.dsc	\
  mul_si.dsc mul_ui.dsc neg.dsc norm.dsc pow.dsc pow_d.dsc pow_fr.dsc	\
  pow_si.dsc pow_ui.dsc pow_z.dsc proj.dsc real.dsc rootofunity.dsc     \
  sin.dsc sin_cos.dsc	                                                \
  sinh.dsc sqr.dsc sqrt.dsc sub.dsc sub_fr.dsc sub_ui.dsc tan.dsc	\
  tanh.dsc ui_div.dsc ui_ui_sub.dsc
DATA_SETS = abs.dat acos.dat acosh.dat add.dat add_fr.dat arg.dat	\
  asin.dat asinh.dat atan.dat atanh.dat                                 \
  cmp_abs.dat conj.dat cos.dat cosh.dat	                                \
  div.dat div_fr.dat exp.dat fma.dat fr_div.dat fr_sub.dat inp_str.dat	\
  log.dat log10.dat mul.dat mul_fr.dat neg.dat norm.dat pow.dat		\
  pow_fr.dat pow_si.dat pow_ui.dat pow_z.dat proj.dat rootofunity.dat   \
  sin.dat sinh.dat                                                      \
  sqr.dat sqrt.dat strtoc.dat sub.dat sub_fr.dat tan.dat tanh.dat
EXTRA_DIST = data_check.tpl tgeneric.tpl $(DATA_SETS) $(DESCRIPTIONS)

LOG_COMPILER = $(VALGRIND)
AM_LOG_FLAGS = $(VALGRIND_OPTS)
TESTS = $(check_PROGRAMS)
CLEANFILES = mpc_test
