/*	$NetBSD: linit.c,v 1.1.1.1.6.2 2014/05/22 14:09:34 yamt Exp $	*/

/*
** $Id: linit.c,v 1.1.1.1.6.2 2014/05/22 14:09:34 yamt Exp $
** Initialization of libraries for lua.c
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"


static const luaL_Reg lualibs[] = {
  {"", luaopen_base},
#ifndef _KERNEL
  {LUA_LOADLIBNAME, luaopen_package},
#endif
  {LUA_TABLIBNAME, luaopen_table},
#ifndef _KERNEL
  {LUA_IOLIBNAME, luaopen_io},
  {LUA_OSLIBNAME, luaopen_os},
#endif
  {LUA_STRLIBNAME, luaopen_string},
#ifndef _KERNEL
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_DBLIBNAME, luaopen_debug},
#endif
  {NULL, NULL}
};


LUALIB_API void luaL_openlibs (lua_State *L) {
  const luaL_Reg *lib = lualibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }
}

