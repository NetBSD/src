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
/*	The \fIcontext\fR argument specifies what kind of names and
/*	masks are being manipulated, in order to make error messages
/*	more understandable. Typically, this would be the name of a
/*	user-configurable parameter.
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
#include <vstring.h>

#define STR(x) vstring_str(x)

/* name_mask - compute mask corresponding to list of names */

int     name_mask(const char *context, NAME_MASK * table, const char *names)
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
	    if (np->name == 0)
		msg_fatal("unknown %s value \"%s\" in \"%s\"",
			  context, name, names);
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

/* str_name_mask - mask to string */

const char *str_name_mask(const char *context, NAME_MASK * table, int mask)
{
    char   *myname = "name_mask";
    NAME_MASK *np;
    int     len;
    static VSTRING *buf = 0;

    if (buf == 0)
	buf = vstring_alloc(1);

    VSTRING_RESET(buf);

    for (np = table; mask != 0; np++) {
	if (np->name == 0)
	    msg_panic("%s: invalid %s bitmask: 0x%x", myname, context, mask);
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
