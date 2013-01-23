/*	$NetBSD: dict_cache.h,v 1.1.1.1.8.1 2013/01/23 00:05:15 yamt Exp $	*/

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

 /*
  * External interface.
  */
typedef struct DICT_CACHE DICT_CACHE;
typedef int (*DICT_CACHE_VALIDATOR_FN) (const char *, const char *, char *);

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

#define DICT_CACHE_CTL_END		0	/* list terminator */
#define DICT_CACHE_CTL_FLAGS		1	/* see above */
#define DICT_CACHE_CTL_INTERVAL		2	/* cleanup interval */
#define DICT_CACHE_CTL_VALIDATOR	3	/* call-back validator */
#define DICT_CACHE_CTL_CONTEXT		4	/* call-back context */

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
