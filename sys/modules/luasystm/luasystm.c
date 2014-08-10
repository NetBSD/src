/*	$NetBSD: luasystm.c,v 1.2.2.1 2014/08/10 06:56:10 tls Exp $ */

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

/* Lua systm module */

#include <sys/param.h>
#include <sys/lua.h>
#include <sys/callout.h>
#include <sys/cpu.h>
#ifdef _MODULE
#include <sys/module.h>
#endif
#include <sys/systm.h>

#include <lua.h>
#include <lauxlib.h>

#ifdef _MODULE
MODULE(MODULE_CLASS_MISC, luasystm, "lua");

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
systm_aprint_normal(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_normal("%s", s);
	return 0;
}

static int
systm_aprint_naive(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_naive("%s", s);
	return 0;
}

static int
systm_aprint_verbose(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_verbose("%s", s);
	return 0;
}

static int
systm_aprint_debug(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_debug("%s", s);
	return 0;
}

static int
systm_aprint_error(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		aprint_error("%s", s);
	return 0;
}

static int
systm_aprint_get_error_count(lua_State *L)
{
	lua_pushinteger(L, aprint_get_error_count());
	return 1;
}

/* panicing */

static int
systm_panic(lua_State *L)
{
	const char *s;

	s = lua_tostring(L, -1);
	if (s)
		panic("%s", s);
	return 0;
}

/* callouts */

/* mutexes */

static int
luaopen_systm(lua_State *L)
{
	const luaL_Reg systm_lib[ ] = {
		{ "print",			print },
		{ "print_nolog",		print_nolog },
		{ "uprint",			uprint },
		{ "aprint_normal",		systm_aprint_normal },
		{ "aprint_naive",		systm_aprint_naive },
		{ "aprint_verbose",		systm_aprint_verbose },
		{ "aprint_debug",		systm_aprint_debug },
		{ "aprint_error",		systm_aprint_error },
		{ "aprint_get_error_count",	systm_aprint_get_error_count },

		/* panicing */
		{ "panic",			systm_panic },

		/* callouts */

		/* mutexes */

		{NULL, NULL}
	};

	luaL_newlib(L, systm_lib);

	/* some string values */
	lua_pushstring(L, copyright);
	lua_setfield(L, -2, "copyright");
	lua_pushstring(L, cpu_getmodel());
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

	return 1;
}

static int
luasystm_modcmd(modcmd_t cmd, void *opaque)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = klua_mod_register("systm", luaopen_systm);
		break;
	case MODULE_CMD_FINI:
		error = klua_mod_unregister("systm");
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
#endif
