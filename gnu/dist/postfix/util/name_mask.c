/*++
/* NAME
/*	name_mask 3
/* SUMMARY
/*	map names to bit mask
/* SYNOPSIS
/*	#include <name_mask.h>
/*
/*	int	name_mask(table, names)
/*	NAME_MASK *table;
/*	const char *names;
/* DESCRIPTION
/*	name_mask() takes a null-terminated \fItable\fR with (name, mask)
/*	values and computes the bit-wise OR of the masks that correspond
/*	to the names listed in the \fInames\fR argument, separated by
/*	comma and/or whitespace characters.
/* DIAGNOSTICS
/*	Fatal: the \fInames\fR argument specifies a name not found in
/*	\fItable\fR.
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
#include <stringops.h>
#include <name_mask.h>

/* name_mask - compute mask corresponding to list of names */

int     name_mask(NAME_MASK *table, const char *names)
{
    const char *myname = "name_mask";
    char   *saved_names = mystrdup(names);
    char   *bp = saved_names;
    int     result = 0;
    NAME_MASK *np;
    char   *name;

    /*
     * Break up the names string, and look up each component in the table. If
     * the name is found, merge its mask with the result.
     */
    while ((name = mystrtok(&bp, ", \t\r\n")) != 0) {
	for (np = table; /* void */ ; np++) {
	    if (np->name == 0)
		msg_fatal("unknown name \"%s\" in \"%s\"", name, names);
	    if (strcmp(name, np->name) == 0) {
		if (msg_verbose)
		    msg_info("%s: %s", myname, name);
		result |= np->mask;
		break;
	    }
	}
    }
    myfree(saved_names);
    return (result);
}
