/*	$NetBSD: map_search.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

/*++
/* NAME
/*      map_search 3h
/* SUMMARY
/*      lookup table search list support
/* SYNOPSIS
/*      #include <map_search.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <name_code.h>

 /*
  * External interface.
  * 
  * The map_search module maintains one lookup table with MAP_SEARCH results,
  * indexed by the unparsed form of a map specification. The conversion from
  * unparsed form to MAP_SEARCH result is controlled by a NAME_CODE mapping,
  * Since one lookup table can support only one mapping per unparsed name,
  * every MAP_SEARCH result in the lookup table must be built using the same
  * NAME_CODE table.
  * 
  * Alternative 1: no lookup table. Allow the user to specify the NAME_CODE
  * mapping in the map_search_create() request (in addition to the unparsed
  * form), and let the MAP_SEARCH user store each MAP_SEARCH pointer. But
  * that would clumsify code that wants to use MAP_SEARCH functionality.
  * 
  * Alternative 2: one lookup table per NAME_CODE mapping. Change
  * map_search_init() to return a pointer to {HTABLE *, NAME_CODE *}, and
  * require that the MAP_SEARCH user pass that pointer to other
  * map_search_xxx() calls (in addition to the unparsed forms). That would be
  * about as clumsy as Alternative 1.
  * 
  * Alternative 3: one lookup table, distinct lookup keys per NAME_CODE table
  * and map_spec. The caller specifies both the map_spec and the NAME_CODE
  * mapping when it calls map_seach_create() and map_search_find(). The
  * implementation securely prepends the name_code argument to the map_spec
  * argument and uses the result as the table lookup key.
  * 
  * Alternative 1 is not suitable for the smtpd_mumble_restrictions parser,
  * which instantiates MAP_SEARCH instances without knowing which specific
  * access feature is involved. It uses a NAME_CODE mapping that contains the
  * superset of what all smtpd_mumble_restrictions features need. The
  * downside is delayed error notification.
  */
typedef struct {
    char   *map_type_name;		/* "type:name", owned */
    char   *search_order;		/* null or owned */
} MAP_SEARCH;

extern void map_search_init(const NAME_CODE *);
extern const MAP_SEARCH *map_search_create(const char *);
extern const MAP_SEARCH *map_search_lookup(const char *);

#define MAP_SEARCH_ATTR_NAME_SEARCH	"search_order"

#define MAP_SEARCH_CODE_UNKNOWN		127

/* LICENSE
/* .ad
/* .fi
/*      The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      Google, Inc.
/*      111 8th Avenue
/*      New York, NY 10011, USA
/*--*/
