#  @(#)Makefile 5.1beta 93/09/24 
#  $Id: Makefile,v 1.11 1994/02/11 18:35:35 jtc Exp $
# 
#  ====================================================
#  Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
# 
#  Developed at SunPro, a Sun Microsystems, Inc. business.
#  Permission to use, copy, modify, and distribute this
#  software is freely granted, provided that this notice 
#  is preserved.
#  ====================================================
# 
# 

#
# There are two options in making libm at fdlibm compile time:
# 	_IEEE_LIBM 	--- IEEE libm; smaller, and somewhat faster
#	_MULTI_LIBM	--- Support multi-standard at runtime by 
#			    imposing wrapper functions defined in 
#			    fdlibm.h:
#				_IEEE_MODE 	-- IEEE
#				_XOPEN_MODE 	-- X/OPEN
#				_POSIX_MODE 	-- POSIX/ANSI
#				_SVID3_MODE 	-- SVID
#
# Here is how to set up CFLAGS to create the desired libm at 
# compile time:
#
# 	CFLAGS = -D_IEEE_LIBM		... IEEE libm (recommended)
#	CFLAGS = -D_SVID3_MODE	... Multi-standard supported
#					    libm with SVID as the 
#					    default standard
#	CFLAGS = -D_XOPEN_MODE	... Multi-standard supported
#					    libm with XOPEN as the 
#					    default standard
#	CFLAGS = -D_POSIX_MODE	... Multi-standard supported
#					    libm with POSIX as the 
#					    default standard
#	CFLAGS = 			... Multi-standard supported
#					    libm with IEEE as the 
#					    default standard
# 

.if (${MACHINE_ARCH} == "i386")
# Uncomment the following lines if you have a i387 (or i486 or Pentium)
#.PATH:	${.CURDIR}/arch/i387
.elif (${MACHINE_ARCH} == "m68k")
#.PATH:	${.CURDIR}/arch/mc68881
.endif

.PATH:	${.CURDIR}/man
.PATH:	${.CURDIR}/src


CFLAGS+= -D_MULTI_LIBM -D_POSIX_MODE
NOPIC=

LIB=	m
OBJS = 	k_standard.o k_rem_pio2.o k_cos.o k_sin.o k_tan.o \
	e_acos.o e_acosh.o e_asin.o e_atan2.o e_atanh.o e_cosh.o e_exp.o \
	e_fmod.o e_gamma.o e_gamma_r.o e_hypot.o e_j0.o e_j1.o e_jn.o \
	e_lgamma.o e_lgamma_r.o e_log.o e_log10.o e_pow.o e_rem_pio2.o \
	e_remainder.o e_scalb.o e_sinh.o e_sqrt.o \
	w_acos.o w_acosh.o w_asin.o w_atan2.o \
	w_atanh.o w_cosh.o w_exp.o w_fmod.o \
	w_gamma.o w_gamma_r.o w_hypot.o w_j0.o \
	w_j1.o w_jn.o w_lgamma.o w_lgamma_r.o \
	w_log.o w_log10.o w_pow.o w_remainder.o \
	w_scalb.o w_sinh.o w_sqrt.o \
	s_asinh.o s_atan.o s_cbrt.o s_ceil.o s_copysign.o \
	s_cos.o s_erf.o s_expm1.o s_fabs.o s_finite.o s_floor.o \
	s_frexp.o s_ilogb.o s_isnan.o s_ldexp.o s_lib_version.o \
	s_log1p.o s_logb.o s_matherr.o s_modf.o s_nextafter.o \
	s_rint.o s_scalbn.o s_signgam.o s_significand.o s_sin.o \
	s_tan.o s_tanh.o


MAN3+=	acos.0 acosh.0 asin.0 asinh.0 atan.0 atan2.0 atanh.0 ceil.0 \
	cos.0 cosh.0 erf.0 exp.0 fabs.0 floor.0 fmod.0 hypot.0 ieee.0 \
	infnan.0 j0.0 lgamma.0 math.0 rint.0 sin.0 sinh.0 sqrt.0 \
	tan.0 tanh.0

MLINKS+=erf.3 erfc.3
MLINKS+=exp.3 expm1.3 exp.3 log.3 exp.3 log10.3 exp.3 log1p.3 exp.3 pow.3
MLINKS+=hypot.3 cabs.3
MLINKS+=ieee.3 copysign.3 ieee.3 drem.3 ieee.3 finite.3 ieee.3 logb.3 \
	ieee.3 scalb.3
MLINKS+=j0.3 j1.3 j0.3 jn.3 j0.3 y0.3 j0.3 y1.3 j0.3 yn.3
MLINKS+=lgamma.3 gamma.3
MLINKS+=sqrt.3 cbrt.3


.include <bsd.lib.mk>
