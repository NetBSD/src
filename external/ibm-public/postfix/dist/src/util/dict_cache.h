/*	$NetBSD: dict_cache.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _DICT_CACHE_H_INCLUDED_
#define _DICT_CACHE_H_INCLUDED_

/*++
/* NAME
/*	dict_cache 3h
/* SUMMARY
/*	External cache manager
/* SYNOPSIS
/*	#include <dict_cache.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>
#include <check_arg.h>

 /*
  * External interface.
  */
typedef struct DICT_CACHE DICT_CACHE;
typedef int (*DICT_CACHE_VALIDATOR_FN) (const char *, const char *, void *);

extern DICT_CACHE *dict_cache_open(const char *, int, int);
extern void dict_cache_close(DICT_CACHE *);
extern const char *dict_cache_lookup(DICT_CACHE *, const char *);
extern int dict_cache_update(DICT_CACHE *, const char *, const char *);
extern int dict_cache_delete(DICT_CACHE *, const char *);
extern int dict_cache_sequence(DICT_CACHE *, int, const char **, const char **);
extern void dict_cache_control(DICT_CACHE *,...);
extern const char *dict_cache_name(DICT_CACHE *);

#define DICT_CACHE_FLAG_VERBOSE		(1<<0)	/* verbose operation */
#define DICT_CACHE_FLAG_STATISTICS	(1<<1)	/* log cache statistics */

/* Legacy API: type-unchecked argument, internal use. */
#define DICT_CACHE_CTL_END		0	/* list terminator */
#define DICT_CACHE_CTL_FLAGS		1	/* see above */
#define DICT_CACHE_CTL_INTERVAL		2	/* cleanup interval */
#define DICT_CACHE_CTL_VALIDATOR	3	/* call-back validator */
#define DICT_CACHE_CTL_CONTEXT		4	/* call-back context */

/* Safer API: type-checked arguments, external use. */
#define CA_DICT_CACHE_CTL_END		DICT_CACHE_CTL_END
#define CA_DICT_CACHE_CTL_FLAGS(v)	DICT_CACHE_CTL_FLAGS, CHECK_VAL(DICT_CACHE, int, (v))
#define CA_DICT_CACHE_CTL_INTERVAL(v)	DICT_CACHE_CTL_INTERVAL, CHECK_VAL(DICT_CACHE, int, (v))
#define CA_DICT_CACHE_CTL_VALIDATOR(v)	DICT_CACHE_CTL_VALIDATOR, CHECK_VAL(DICT_CACHE, DICT_CACHE_VALIDATOR_FN, (v))
#define CA_DICT_CACHE_CTL_CONTEXT(v)	DICT_CACHE_CTL_CONTEXT, CHECK_PTR(DICT_CACHE, void, (v))

CHECK_VAL_HELPER_DCL(DICT_CACHE, int);
CHECK_VAL_HELPER_DCL(DICT_CACHE, DICT_CACHE_VALIDATOR_FN);
CHECK_PTR_HELPER_DCL(DICT_CACHE, void);

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

#endif
