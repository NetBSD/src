/*	$NetBSD: postscreen_dict.c,v 1.1.1.1 2011/03/02 19:32:26 tron Exp $	*/

/*++
/* NAME
/*	postscreen_dict 3
/* SUMMARY
/*	postscreen table access wrappers
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	int	psc_addr_match_list_match(match_list, client_addr)
/*	ADDR_MATCH_LIST *match_list;
/*	const char *client_addr;
/*
/*	const char *psc_cache_lookup(DICT_CACHE *cache, const char *key)
/*	DICT_CACHE *cache;
/*	const char *key;
/*
/*	void	psc_cache_update(cache, key, value)
/*	DICT_CACHE *cache;
/*	const char *key;
/*	const char *value;
/*
/*	void	psc_dict_get(dict, key)
/*	DICT	*dict;
/*	const char *key;
/*
/*	void	psc_maps_find(maps, key, flags)
/*	MAPS	*maps;
/*	const char *key;
/*	int	flags;
/* DESCRIPTION
/*	This module implements wrappers around time-critical table
/*	access functions.  The functions log a warning when table
/*	access takes a non-trivial amount of time.
/*
/*	psc_addr_match_list_match() is a wrapper around
/*	addr_match_list_match().
/*
/*	psc_cache_lookup() and psc_cache_update() are wrappers around
/*	the corresponding dict_cache() methods.
/*
/*	psc_dict_get() and psc_maps_find() are wrappers around
/*	dict_get() and maps_find(), respectively.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <dict.h>

/* Global library. */

#include <maps.h>

/* Application-specific. */

#include <postscreen.h>

 /*
  * Monitor time-critical operations.
  * 
  * XXX Averaging support was added during a stable release candidate, so it
  * provides only the absolute minimum necessary. A complete implementation
  * should maintain separate statistics for each table, and it should not
  * complain when the access latency is less than the time between accesses.
  */
#define PSC_GET_TIME_BEFORE_LOOKUP { \
    struct timeval _before, _after; \
    DELTA_TIME _delta; \
    double _new_delta_ms; \
    GETTIMEOFDAY(&_before);

#define PSC_DELTA_MS(d) ((d).dt_sec * 1000.0 + (d).dt_usec / 1000.0)

#define PSC_AVERAGE(new, old)	(0.1 * (new) + 0.9 * (old))

#ifndef PSC_THRESHOLD_MS
#define PSC_THRESHOLD_MS	100	/* nag if latency > 100ms */
#endif

#ifndef PSC_WARN_LOCKOUT_S
#define PSC_WARN_LOCKOUT_S	60	/* don't nag for 60s */
#endif

 /*
  * Shared warning lock, so that we don't spam the logfile when the system
  * becomes slow.
  */
static time_t psc_last_warn = 0;

#define PSC_CHECK_TIME_AFTER_LOOKUP(table, action, average) \
    GETTIMEOFDAY(&_after); \
    PSC_CALC_DELTA(_delta, _after, _before); \
    _new_delta_ms = PSC_DELTA_MS(_delta); \
    if ((average = PSC_AVERAGE(_new_delta_ms, average)) > PSC_THRESHOLD_MS \
	&& psc_last_warn < _after.tv_sec - PSC_WARN_LOCKOUT_S) { \
        msg_warn("%s: %s %s average delay is %.0f ms", \
                 myname, (table), (action), average); \
	psc_last_warn = _after.tv_sec; \
    } \
}

/* psc_addr_match_list_match - time-critical address list lookup */

int     psc_addr_match_list_match(ADDR_MATCH_LIST *addr_list,
				          const char *addr_str)
{
    const char *myname = "psc_addr_match_list_match";
    int     result;
    static double latency_ms;

    PSC_GET_TIME_BEFORE_LOOKUP;
    result = addr_match_list_match(addr_list, addr_str);
    PSC_CHECK_TIME_AFTER_LOOKUP("address list", "lookup", latency_ms);
    return (result);
}

/* psc_cache_lookup - time-critical cache lookup */

const char *psc_cache_lookup(DICT_CACHE *cache, const char *key)
{
    const char *myname = "psc_cache_lookup";
    const char *result;
    static double latency_ms;

    PSC_GET_TIME_BEFORE_LOOKUP;
    result = dict_cache_lookup(cache, key);
    PSC_CHECK_TIME_AFTER_LOOKUP(dict_cache_name(cache), "lookup", latency_ms);
    return (result);
}

/* psc_cache_update - time-critical cache update */

void    psc_cache_update(DICT_CACHE *cache, const char *key, const char *value)
{
    const char *myname = "psc_cache_update";
    static double latency_ms;

    PSC_GET_TIME_BEFORE_LOOKUP;
    dict_cache_update(cache, key, value);
    PSC_CHECK_TIME_AFTER_LOOKUP(dict_cache_name(cache), "update", latency_ms);
}

/* psc_dict_get - time-critical table lookup */

const char *psc_dict_get(DICT *dict, const char *key)
{
    const char *myname = "psc_dict_get";
    const char *result;
    static double latency_ms;

    PSC_GET_TIME_BEFORE_LOOKUP;
    result = dict_get(dict, key);
    PSC_CHECK_TIME_AFTER_LOOKUP(dict->name, "lookup", latency_ms);
    return (result);
}

/* psc_maps_find - time-critical table lookup */

const char *psc_maps_find(MAPS *maps, const char *key, int flags)
{
    const char *myname = "psc_maps_find";
    const char *result;
    static double latency_ms;

    PSC_GET_TIME_BEFORE_LOOKUP;
    result = maps_find(maps, key, flags);
    PSC_CHECK_TIME_AFTER_LOOKUP(maps->title, "lookup", latency_ms);
    return (result);
}
