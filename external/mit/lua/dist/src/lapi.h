/*	$NetBSD: lapi.h,v 1.1.1.1.6.1 2012/04/17 00:04:46 yamt Exp $	*/

/*
** $Id: lapi.h,v 1.1.1.1.6.1 2012/04/17 00:04:46 yamt Exp $
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/

#ifndef lapi_h
#define lapi_h


#include "lobject.h"


LUAI_FUNC void luaA_pushobject (lua_State *L, const TValue *o);

#endif
