/*	$NetBSD: luapmf.c,v 1.3.2.1 2014/08/10 06:56:10 tls Exp $ */

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

/* Lua pmf module */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/lua.h>
#ifdef _MODULE
#include <sys/module.h>
#endif
#include <sys/reboot.h>

#include <lua.h>
#include <lauxlib.h>

#ifdef _MODULE
MODULE(MODULE_CLASS_MISC, luapmf, "lua");

static int
system_shutdown(lua_State *L)
{
	pmf_system_shutdown(lua_tointeger(L, 1));
	return 0;
}

static int
set_platform(lua_State *L)
{
	const char *key, *value;

	key = lua_tostring(L, -2);
	value = lua_tostring(L, -1);
	if (key != NULL && value != NULL)
		pmf_set_platform(key, value);
	return 0;
}

static int
get_platform(lua_State *L)
{
	const char *key, *value;

	key = lua_tostring(L, -1);
	if (key != NULL) {
		value = pmf_get_platform(key);
		if (value != NULL)
			lua_pushstring(L, value);
		else
			lua_pushnil(L);
	} else
		lua_pushnil(L);
	return 1;

}

static int
luaopen_pmf(lua_State *L)
{
	const luaL_Reg pmf_lib[ ] = {
		{ "system_shutdown",	system_shutdown },
		{ "set_platform",	set_platform },
		{ "get_platform",	get_platform },
		{ NULL, NULL }
	};

	luaL_newlib(L, pmf_lib);

	/* some integer values */
	lua_pushinteger(L, PMFE_DISPLAY_ON);
	lua_setfield(L, -2, "PMFE_DISPLAY_ON");
	lua_pushinteger(L, PMFE_DISPLAY_REDUCED);
	lua_setfield(L, -2, "PMFE_DISPLAY_REDUCED");
	lua_pushinteger(L, PMFE_DISPLAY_STANDBY);
	lua_setfield(L, -2, "PMFE_DISPLAY_STANDBY");
	lua_pushinteger(L, PMFE_DISPLAY_SUSPEND);
	lua_setfield(L, -2, "PMFE_DISPLAY_SUSPEND");
	lua_pushinteger(L, PMFE_DISPLAY_OFF);
	lua_setfield(L, -2, "PMFE_DISPLAY_OFF");
	lua_pushinteger(L, PMFE_DISPLAY_BRIGHTNESS_UP);
	lua_setfield(L, -2, "PMFE_DISPLAY_BRIGHTNESS_UP");
	lua_pushinteger(L, PMFE_DISPLAY_BRIGHTNESS_DOWN);
	lua_setfield(L, -2, "PMFE_DISPLAY_BRIGHTNESS_DOWN");
	lua_pushinteger(L, PMFE_AUDIO_VOLUME_UP);
	lua_setfield(L, -2, "PMFE_AUDIO_VOLUME_UP");
	lua_pushinteger(L, PMFE_AUDIO_VOLUME_DOWN);
	lua_setfield(L, -2, "PMFE_AUDIO_VOLUME_DOWN");
	lua_pushinteger(L, PMFE_AUDIO_VOLUME_TOGGLE);
	lua_setfield(L, -2, "PMFE_AUDIO_VOLUME_TOGGLE");
	lua_pushinteger(L, PMFE_CHASSIS_LID_CLOSE);
	lua_setfield(L, -2, "PMFE_CHASSIS_LID_CLOSE");
	lua_pushinteger(L, PMFE_CHASSIS_LID_OPEN);
	lua_setfield(L, -2, "PMFE_CHASSIS_LID_OPEN");

	/* reboot(2) howto arg */
	lua_pushinteger(L, RB_AUTOBOOT);
	lua_setfield(L, -2, "RB_AUTOBOOT");
	lua_pushinteger(L, RB_ASKNAME);
	lua_setfield(L, -2, "RB_ASKNAME");
	lua_pushinteger(L, RB_DUMP);
	lua_setfield(L, -2, "RB_DUMP");
	lua_pushinteger(L, RB_HALT);
	lua_setfield(L, -2, "RB_HALT");
	lua_pushinteger(L, RB_POWERDOWN);
	lua_setfield(L, -2, "RB_POWERDOWN");
	lua_pushinteger(L, RB_KDB);
	lua_setfield(L, -2, "RB_KDB");
	lua_pushinteger(L, RB_NOSYNC);
	lua_setfield(L, -2, "RB_NOSYNC");
	lua_pushinteger(L, RB_RDONLY);
	lua_setfield(L, -2, "RB_RDONLY");
	lua_pushinteger(L, RB_SINGLE);
	lua_setfield(L, -2, "RB_SINGLE");
	lua_pushinteger(L, RB_USERCONF);
	lua_setfield(L, -2, "RB_USERCONF");

	return 1;
}

static int
luapmf_modcmd(modcmd_t cmd, void *opaque)
{
	int error;
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = klua_mod_register("pmf", luaopen_pmf);
		break;
	case MODULE_CMD_FINI:
		error = klua_mod_unregister("pmf");
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
#endif
