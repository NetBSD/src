/*++
/* NAME
/*	virtual8_maps 3
/* SUMMARY
/*	virtual delivery agent map lookups
/* SYNOPSIS
/*	#include <virtual8_maps.h>
/*
/*	MAPS	*virtual8_maps_create(title, map_names, flags)
/*	const char *title;
/*	const char *map_names;
/*	int	flags;
/*
/*	const char *virtual8_maps_find(maps, recipient)
/*	MAPS	*maps;
/*	const char *recipient;
/*
/*	MAPS	*virtual8_maps_free(maps)
/*	MAPS	*maps;
/* DESCRIPTION
/*	This module does user lookups for the virtual delivery
/*	agent. The code is made available as a library module so that
/*	other programs can perform compatible queries.
/*
/*	Lookups are case sensitive.
/*
/*	virtual8_maps_create() takes list of type:name pairs and opens the
/*	named dictionaries.
/*	The result is a handle that must be specified along with all
/*	other virtual8_maps_xxx() operations.
/*	See dict_open(3) for a description of flags.
/*
/*	virtual8_maps_find() searches the specified list of dictionaries
/*	in the specified order for the named key. The result is in
/*	memory that is overwritten upon each call.
/*
/*	virtual8_maps_free() releases storage claimed by virtual8_maps_create()
/*	and conveniently returns a null pointer.
/*
/*	Arguments:
/* .IP title
/*	String used for diagnostics. Typically one specifies the
/*	type of information stored in the lookup tables.
/* .IP map_names
/*	Null-terminated string with type:name dictionary specifications,
/*	separated by whitespace or commas.
/* .IP maps
/*	A result from maps_create().
/* .IP key
/*	Null-terminated string with a lookup key. Table lookup is case
/*	sensitive.
/* DIAGNOSTICS
/*	The dict_errno variable is non-zero in case of problems.
/* BUGS
/*	This code is a temporary solution that implements a hard-coded
/*	lookup strategy. In a future version of Postfix, the lookup
/*	strategy should become configurable.
/* SEE ALSO
/*	virtual(8) virtual mailbox delivery agent
/*	maps(3) multi-dictionary search
/*	dict_open(3) low-level dictionary interface
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
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>

/* Global library. */

#include <maps.h>
#include <mail_params.h>
#include <strip_addr.h>
#include <virtual8_maps.h>

/* Application-specific. */

/* virtual8_maps_find - lookup for virtual delivery agent */

const char *virtual8_maps_find(MAPS *maps, const char *recipient)
{
    const char *ratsign;
    const char *result;
    char   *bare = 0;

    /*
     * Look up the address minus the optional extension. This is done first,
     * to avoid hammering the database with extended address lookups, and to
     * have straightforward semantics (extensions are always ignored).
     */
    if (*var_rcpt_delim
     && (bare = strip_addr(recipient, (char **) 0, *var_rcpt_delim)) != 0) {
	result = maps_find(maps, bare, DICT_FLAG_FIXED);
	myfree(bare);
	if (result != 0 || dict_errno != 0)
	    return (result);
    }

    /*
     * Look up the full address. Allow regexp table searches.
     */
    if (bare == 0) {
	result = maps_find(maps, recipient, DICT_FLAG_NONE);
	if (result != 0 || dict_errno != 0)
	    return (result);
    }

    /*
     * Look up the @domain catch-all.
     */
    if ((ratsign = strrchr(recipient, '@')) == 0)
	return (0);
    return (maps_find(maps, ratsign, DICT_FLAG_FIXED));
}

#ifdef TEST

#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>

#define STR(x)	vstring_str(x)

int     main(int argc, char **argv)
{
    VSTRING *buffer;
    MAPS   *maps;
    const char *result;

    if (argc != 2)
	msg_fatal("usage: %s mapname", argv[0]);

    var_rcpt_delim = "+";
    var_double_bounce_sender = DEF_DOUBLE_BOUNCE;

    maps = virtual8_maps_create("testmap", argv[1], DICT_FLAG_LOCK);
    buffer = vstring_alloc(1);

    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	result = virtual8_maps_find(maps, STR(buffer));
	vstream_printf("%s -> %s\n", STR(buffer), result ? result : "(none)");
	vstream_fflush(VSTREAM_OUT);
    }
    virtual8_maps_free(maps);
    vstring_free(buffer);
    return (0);
}

#endif
