/*	$NetBSD: ldebug.h,v 1.2 2014/07/19 18:38:34 lneto Exp $	*/

/*
** $Id: ldebug.h,v 1.2 2014/07/19 18:38:34 lneto Exp $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in lua.h
*/

#ifndef ldebug_h
#define ldebug_h


#include "lstate.h"


#define pcRel(pc, p)	(cast(int, (pc) - (p)->code) - 1)

#define getfuncline(f,pc)	(((f)->lineinfo) ? (f)->lineinfo[pc] : -1)

#define resethookcount(L)	(L->hookcount = L->basehookcount)

/* Active Lua function (given call info) */
#define ci_func(ci)		(clLvalue((ci)->func))


LUAI_FUNC l_noret luaG_typeerror (lua_State *L, const TValue *o,
                                                const char *opname);
LUAI_FUNC l_noret luaG_concaterror (lua_State *L, const TValue *p1,
                                                  const TValue *p2);
LUAI_FUNC l_noret luaG_aritherror (lua_State *L, const TValue *p1,
                                                 const TValue *p2);
LUAI_FUNC l_noret luaG_tointerror (lua_State *L, const TValue *p1,
                                                 const TValue *p2);
LUAI_FUNC l_noret luaG_ordererror (lua_State *L, const TValue *p1,
                                                 const TValue *p2);
LUAI_FUNC l_noret luaG_runerror (lua_State *L, const char *fmt, ...);
LUAI_FUNC l_noret luaG_errormsg (lua_State *L);
LUAI_FUNC void luaG_traceexec (lua_State *L);


#endif
