/*	$NetBSD: luacore.c,v 1.4 2013/10/23 18:57:40 mbalmer Exp $ */

/*
 * Copyright (c) 2011, 2013 Marc Balmer <mbalmer@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Lua core kernel services module */

#include <sys/param.h>
#include <sys/lua.h>
#include <sys/callout.h>
#ifdef _MODULE
#include <sys/module.h>
#endif
#include <sys/systm.h>

#include <lua.h>

#ifdef _MODULE
MODULE(MODULE_CLASS_MISC, luacore, "lua");

/* Various printing functions */
static int
print(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		printf("%s", s);
	return 0;
}

static int
print_nolog(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		printf_nolog("%s", s);
	return 0;
}

static int
uprint(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		uprintf("%s", s);
	return 0;
}

static int
core_aprint_normal(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_normal("%s", s);
	return 0;
}

static int
core_aprint_naive(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_naive("%s", s);
	return 0;
}

static int
core_aprint_verbose(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_verbose("%s", s);
	return 0;
}

static int
core_aprint_debug(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_debug("%s", s);
	return 0;
}

static int
core_aprint_error(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_error("%s", s);
	return 0;
}

static int
core_aprint_get_error_count(lua_State *L)
{
	lua_pushinteger(L, aprint_get_error_count());
	return 1;
}

/* panicing */

static int
core_panic(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		panic("%s", s);
	return 0;
}

/* callouts */

/* mutexes */

struct core_reg {
	const char *n;
	int (*f)(lua_State *);
};

static int
luaopen_core(void *ls)
{
	lua_State *L = (lua_State *)ls;
	int n, nfunc;
	struct core_reg core[] = {
		/* printing functions */
		{ "print",			print },
		{ "print_nolog",		print_nolog },
		{ "uprint",			uprint },
		{ "aprint_normal",		core_aprint_normal },
		{ "aprint_naive",		core_aprint_naive },
		{ "aprint_verbose",		core_aprint_verbose },
		{ "aprint_debug",		core_aprint_debug },
		{ "aprint_error",		core_aprint_error },
		{ "aprint_get_error_count",	core_aprint_get_error_count },

		/* panicing */
		{ "panic",			core_panic },

		/* callouts */

		/* mutexes */
	};

	nfunc = sizeof(core)/sizeof(core[1]);

	lua_createtable(L, nfunc, 0);
	for (n = 0; n < nfunc; n++) {
		lua_pushcfunction(L, core[n].f);
		lua_setfield(L, -2, core[n].n);
	}

	/* some string values */
	lua_pushstring(L, copyright);
	lua_setfield(L, -2, "copyright");
	lua_pushstring(L, cpu_model);
	lua_setfield(L, -2, "cpu_model");
	lua_pushstring(L, machine);
	lua_setfield(L, -2, "machine");
	lua_pushstring(L, machine_arch);
	lua_setfield(L, -2, "machine_arch");
	lua_pushstring(L, osrelease);
	lua_setfield(L, -2, "osrelease");
	lua_pushstring(L, ostype);
	lua_setfield(L, -2, "ostype");
	lua_pushstring(L, kernel_ident);
	lua_setfield(L, -2, "kernel_ident");
	lua_pushstring(L, version);
	lua_setfield(L, -2, "version");

	/* some integer values */
	lua_pushinteger(L, ncpu);
	lua_setfield(L, -2, "ncpu");

	lua_setglobal(L, "core");
	return 1;
}

static int
luacore_modcmd(modcmd_t cmd, void *opaque)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = lua_mod_register("core", luaopen_core);
		break;
	case MODULE_CMD_FINI:
		error = lua_mod_unregister("core");
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
#endif
