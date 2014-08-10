/*	$NetBSD: lapi.h,v 1.1.1.2.8.1 2014/08/10 06:50:54 tls Exp $	*/

/*
** $Id: lapi.h,v 1.1.1.2.8.1 2014/08/10 06:50:54 tls Exp $
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/

#ifndef lapi_h
#define lapi_h


#include "llimits.h"
#include "lstate.h"

#define api_incr_top(L)   {L->top++; api_check(L, L->top <= L->ci->top, \
				"stack overflow");}

#define adjustresults(L,nres) \
    { if ((nres) == LUA_MULTRET && L->ci->top < L->top) L->ci->top = L->top; }

#define api_checknelems(L,n)	api_check(L, (n) < (L->top - L->ci->func), \
				  "not enough elements in the stack")


#endif
