/* $NetBSD: libm.c,v 1.2 2022/11/23 18:15:43 christos Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Phillip Rulon
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: libm.c,v 1.2 2022/11/23 18:15:43 christos Exp $");

#include <lua.h>
#include <lauxlib.h>
#include <math.h>

const char	badarg[] = "argument to libm function not a number";

/*-
 * The majority of libm functions fall into a few forms:
 *
 *   int func(double);
 *   double func(double);
 *   double func(double, double);
 * and,
 *   double func(int, double);
 *
 * Accordingly, this lends itself to systematic declaration of the lua
 * interface functions.  These macros set this up.
 */
#define BFUNC_DBL(fname)			\
static int					\
libm_##fname(lua_State *L)			\
{						\
	if (!lua_isnumber(L, 1))		\
		return luaL_error(L, badarg);	\
						\
	double x = lua_tonumber(L, 1);		\
	lua_pushboolean(L, fname(x));		\
	return 1;				\
}

#define DFUNC_DBL(fname)			\
static int					\
libm_##fname(lua_State *L)			\
{						\
	if (!lua_isnumber(L, 1)) 		\
    		return luaL_error(L, badarg);	\
						\
	double x = lua_tonumber(L, 1);		\
	lua_pushnumber(L, fname(x));		\
	return 1;				\
}

#define DFUNC_INT_DBL(fname)			\
static int					\
libm_##fname(lua_State *L)			\
{						\
	if (!lua_isinteger(L, 1) ||		\
	    !lua_isnumber(L, 2))		\
    		return luaL_error(L, badarg);	\
						\
	int i = (int)lua_tointeger(L, 1);	\
	double x = lua_tonumber(L, 2);		\
	lua_pushnumber(L, fname(i, x));		\
	return 1;				\
}

#define DFUNC_DBL_DBL(fname)			\
static int					\
libm_##fname(lua_State *L)			\
{						\
	if (!lua_isnumber(L, 1) ||		\
	    !lua_isnumber(L, 2))		\
    		return luaL_error(L, badarg);	\
	double x[] = {				\
	    lua_tonumber(L, 1),			\
	    lua_tonumber(L,2)			\
	};					\
	lua_pushnumber(L, fname(x[0], x[1]));   \
	return 1;				\
}

int	luaopen_libm(lua_State *);

DFUNC_DBL(acos)
DFUNC_DBL(acosh)
DFUNC_DBL(asin)
DFUNC_DBL(asinh)
DFUNC_DBL(atan)
DFUNC_DBL(atanh)
DFUNC_DBL_DBL(atan2)
DFUNC_DBL(cbrt)
DFUNC_DBL(ceil)
DFUNC_DBL_DBL(copysign)
DFUNC_DBL(cos)
DFUNC_DBL(cosh)
DFUNC_DBL(erf)
DFUNC_DBL(erfc)
DFUNC_DBL(exp)
DFUNC_DBL(exp2)
DFUNC_DBL(expm1)
DFUNC_DBL(fabs)
DFUNC_DBL_DBL(fdim)
BFUNC_DBL(finite)
DFUNC_DBL(floor)
DFUNC_DBL_DBL(fmax)
DFUNC_DBL_DBL(fmin)
DFUNC_DBL_DBL(fmod)
DFUNC_DBL(gamma)
DFUNC_DBL_DBL(hypot)
BFUNC_DBL(isfinite)
BFUNC_DBL(isnan)
BFUNC_DBL(isinf)
DFUNC_DBL(j0)
DFUNC_DBL(j1)
DFUNC_INT_DBL(jn)
DFUNC_DBL(lgamma)
DFUNC_DBL(log)
DFUNC_DBL(log10)
DFUNC_DBL(log1p)
#ifndef __vax__
DFUNC_DBL_DBL(nextafter)
#endif
DFUNC_DBL_DBL(pow)
DFUNC_DBL_DBL(remainder)
DFUNC_DBL(rint)
DFUNC_DBL(round)
DFUNC_DBL(sin)
DFUNC_DBL(sinh)
DFUNC_DBL(sqrt)
DFUNC_DBL(tan)
DFUNC_DBL(tanh)
DFUNC_DBL(trunc)
DFUNC_DBL(y0)
DFUNC_DBL(y1)
DFUNC_INT_DBL(yn)

/*
 * The following interface functions are special cases which do not lend
 * themseleves to the systematic declaration scheme above.
 */
static int
libm_fma(lua_State *L)
{
	if (!lua_isnumber(L, 1) ||
	    !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3))
		return luaL_error(L, badarg);

	double	x[] = {
	    lua_tonumber(L, 1),
	    lua_tonumber(L, 2),
	    lua_tonumber(L, 3)
	};
	lua_pushnumber(L, fma(x[0], x[1], x[2]));
	return 1;
}

static int
libm_nan(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return luaL_error(L, badarg);

	const char     *str = luaL_checkstring(L, 1);
	lua_pushnumber(L, nan(str));
	return 1;
}

static int
libm_scalbn(lua_State *L)
{
	if (!lua_isnumber(L, 1) || !lua_isinteger(L, 2))
		return luaL_error(L, badarg);

	double		x = lua_tonumber(L, 1);
	int		i = (int)lua_tointeger(L, 2);
	lua_pushnumber(L, scalbn(x, i));
	return 1;
}

static int
libm_ilogb(lua_State *L)
{
	if (!lua_isnumber(L, 1))
		return luaL_error(L, badarg);

	double		x = lua_tonumber(L, 1);
	lua_pushinteger(L, ilogb(x));
	return 1;
}

/*
 * set up a table for the math.h constants
 */
#define LIBM_CONST(K) {#K, K}
struct kv {
	const char     *k;
	double		v;
};

static const struct kv libm_const[] = {
	LIBM_CONST(M_E),
	LIBM_CONST(M_LOG2E),
	LIBM_CONST(M_LOG10E),
	LIBM_CONST(M_LN2),
	LIBM_CONST(M_LN10),
	LIBM_CONST(M_PI),
	LIBM_CONST(M_PI_2),
	LIBM_CONST(M_PI_4),
	LIBM_CONST(M_1_PI),
	LIBM_CONST(M_2_PI),
	LIBM_CONST(M_2_SQRTPI),
	LIBM_CONST(M_SQRT2),
	LIBM_CONST(M_SQRT1_2),
	{ NULL, 0 }
};


static const struct luaL_Reg lualibm[] = {
	{ "acos", libm_acos },
	{ "acosh", libm_acosh },
	{ "asin", libm_asin },
	{ "asinh", libm_asinh },
	{ "atan", libm_atan },
	{ "atanh", libm_atanh },
	{ "atan2", libm_atan2 },
	{ "cbrt", libm_cbrt },
	{ "ceil", libm_ceil },
	{ "copysign", libm_copysign },
	{ "cos", libm_cos },
	{ "cosh", libm_cosh },
	{ "erf", libm_erf },
	{ "erfc", libm_erfc },
	{ "exp", libm_exp },
	{ "exp2", libm_exp2 },
	{ "expm1", libm_expm1 },
	{ "fabs", libm_fabs },
	{ "fdim", libm_fdim },
	{ "finite", libm_finite },
	{ "floor", libm_floor },
	{ "fma", libm_fma },
	{ "fmax", libm_fmax },
	{ "fmin", libm_fmin },
	{ "fmod", libm_fmod },
	{ "gamma", libm_gamma },
	{ "hypot", libm_hypot },
	{ "ilogb", libm_ilogb },
	{ "isfinite", libm_isfinite },
	{ "isinf", libm_isinf },
	{ "isnan", libm_isnan },
	{ "j0", libm_j0 },
	{ "j1", libm_j1 },
	{ "jn", libm_jn },
	{ "lgamma", libm_lgamma },
	{ "log", libm_log },
	{ "log10", libm_log10 },
	{ "log1p", libm_log1p },
	{ "nan", libm_nan },
#ifndef __vax__
	{ "nextafter", libm_nextafter },
#endif
	{ "pow", libm_pow },
	{ "remainder", libm_remainder },
	{ "rint", libm_rint },
	{ "round", libm_round },
	{ "scalbn", libm_scalbn },
	{ "sin", libm_sin },
	{ "sinh", libm_sinh },
	{ "sqrt", libm_sqrt },
	{ "tan", libm_tan },
	{ "tanh", libm_tanh },
	{ "trunc", libm_trunc },
	{ "y0", libm_y0 },
	{ "y1", libm_y1 },
	{ "yn", libm_yn },
	{ NULL, NULL }
};

int
luaopen_libm(lua_State *L)
{
	const struct kv *kvp = libm_const;

	luaL_newlib(L, lualibm);

	/* integrate the math.h constants */
	while (kvp->k) {
		lua_pushnumber(L, kvp->v);
		lua_setfield(L, -2, kvp->k);
		kvp++;
	}

	return 1;
}
