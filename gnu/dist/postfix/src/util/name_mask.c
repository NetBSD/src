/*	$NetBSD: name_mask.c,v 1.1.1.3 2005/08/18 21:10:32 rpaulo Exp $	*/

/*++
/* NAME
/*	name_mask 3
/* SUMMARY
/*	map names to bit mask
/* SYNOPSIS
/*	#include <name_mask.h>
/*
/*	int	name_mask(context, table, names)
/*	const char *context;
/*	NAME_MASK *table;
/*	const char *names;
/*
/*	const char *str_name_mask(context, table, mask)
/*	const char *context;
/*	NAME_MASK *table;
/*	int	mask;
/*
/*	int	name_mask_opt(context, table, names, flags)
/*	const char *context;
/*	NAME_MASK *table;
/*	const char *names;
/*	int	flags;
/*
/*	const char *str_name_mask_opt(context, table, mask, flags)
/*	const char *context;
/*	NAME_MASK *table;
/*	int	mask;
/*	int	flags;
/* DESCRIPTION
/*	name_mask() takes a null-terminated \fItable\fR with (name, mask)
/*	values and computes the bit-wise OR of the masks that correspond
/*	to the names listed in the \fInames\fR argument, separated by
/*	comma and/or whitespace characters.
/*
/*	str_name_mask() translates a mask into its equivalent names.
/*	The result is written to a static buffer that is overwritten
/*	upon each call.
/*
/*	name_mask_opt() and str_name_mask_opt() are extended versions
/*	with additional fine control.
/*
/*	Arguments:
/* .IP context
/*	What kind of names and
/*	masks are being manipulated, in order to make error messages
/*	more understandable. Typically, this would be the name of a
/*	user-configurable parameter.
/* .IP table
/*	Table with (name, bit mask) pairs.
/* .IP names
/*	A list of names that is to be converted into a bit mask.
/* .IP mask
/*	A bit mask.
/* .IP flags
/*	Bit-wise OR of zero or more of the following:
/* .RS
/* .IP NAME_MASK_MATCH_REQ
/*	Require that all names listed in \fIname\fR exist in \fItable\fR,
/*	and that all bits listed in \fImask\fR exist in \fItable\fR.
/*	This feature is enabled by default when calling name_mask()
/*	or str_name_mask().
/* .IP NAME_MASK_ANY_CASE
/*	Enable case-insensitive matching.
/*	This feature is not enabled by default when calling name_mask();
/*	it has no effect with str_name_mask().
/* .RE
/*	The value NAME_MASK_NONE explicitly requests no features,
/*	and NAME_MASK_DEFAULT enables the default options.
/* DIAGNOSTICS
/*	Fatal: the \fInames\fR argument specifies a name not found in
/*	\fItable\fR, or the \fImask\fR specifies a bit not found in
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <name_mask.h>
#include <vstring.h>

#define STR(x) vstring_str(x)

/* name_mask_opt - compute mask corresponding to list of names */

int     name_mask_opt(const char *context, NAME_MASK *table, const char *names,
		              int flags)
{
    char   *myname = "name_mask";
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
	    if (np->name == 0) {
		if (flags & NAME_MASK_MATCH_REQ)
		    msg_fatal("unknown %s value \"%s\" in \"%s\"",
			      context, name, names);
		break;
	    }
	    if (((flags & NAME_MASK_ANY_CASE) ? strcasecmp : strcmp)
			(name, np->name) == 0) {
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

/* str_name_mask_opt - mask to string */

const char *str_name_mask_opt(const char *context, NAME_MASK *table,
			              int mask, int flags)
{
    char   *myname = "name_mask";
    NAME_MASK *np;
    int     len;
    static VSTRING *buf = 0;

    if (buf == 0)
	buf = vstring_alloc(1);

    VSTRING_RESET(buf);

    for (np = table; mask != 0; np++) {
	if (np->name == 0) {
	    if (flags & NAME_MASK_MATCH_REQ)
		msg_fatal("%s: invalid %s bit in mask: 0x%x",
			  myname, context, mask);
	    break;
	}
	if (mask & np->mask) {
	    mask &= ~np->mask;
	    vstring_sprintf_append(buf, "%s ", np->name);
	}
    }
    if ((len = VSTRING_LEN(buf)) > 0)
	vstring_truncate(buf, len - 1);

    return (STR(buf));
}

#ifdef TEST

 /*
  * Stand-alone test program.
  */

#include <vstream.h>

int     main(int argc, char **argv)
{
    static NAME_MASK table[] = {
	"zero", 1 << 0,
	"one", 1 << 1,
	"two", 1 << 2,
	"three", 1 << 3,
	0, 0,
    };
    int     mask;
    VSTRING *buf = vstring_alloc(1);

    while (--argc && *++argv) {
	mask = name_mask("test", table, *argv);
	vstream_printf("%s -> 0x%x -> %s\n",
		       *argv, mask, str_name_mask("mask_test", table, mask));
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(buf);
    exit(0);
}

#endif
