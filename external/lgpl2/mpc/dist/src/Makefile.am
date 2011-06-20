## src/Makefile.am -- Process this file with automake to produce Makefile.in
AM_CFLAGS=@WARNINGCFLAGS@

lib_LTLIBRARIES = libmpc.la
libmpc_la_LDFLAGS = -version-info 2:0:0
libmpc_la_SOURCES = mpc-impl.h abs.c acos.c acosh.c add.c add_fr.c         \
  add_si.c add_ui.c arg.c asin.c asinh.c atan.c atanh.c clear.c cmp.c cmp_si_si.c   \
  conj.c cosh.c div_2exp.c div.c div_fr.c div_ui.c exp.c fma.c fr_div.c    \
  fr_sub.c get.c get_prec2.c get_prec.c get_str.c get_version.c imag.c     \
  init2.c init3.c inp_str.c log.c mem.c mul_2exp.c mul.c mul_fr.c mul_i.c  \
  mul_si.c mul_ui.c neg.c norm.c out_str.c pow.c pow_fr.c pow_ld.c pow_d.c \
  pow_si.c pow_ui.c pow_z.c proj.c real.c urandom.c set.c set_prec.c       \
  set_str.c set_x.c set_x_x.c sin_cos.c sinh.c sqr.c sqrt.c strtoc.c \
  sub.c sub_fr.c sub_ui.c swap.c tan.c tanh.c uceil_log2.c ui_div.c        \
  ui_ui_sub.c

libmpc_la_LIBADD = @LTLIBOBJS@
