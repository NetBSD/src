/*	$NetBSD: name_mask.c,v 1.1.1.3 2013/01/02 18:59:13 tron Exp $	*/

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
/*	const NAME_MASK *table;
/*	const char *names;
/*
/*	long	long_name_mask(context, table, names)
/*	const char *context;
/*	const LONG_NAME_MASK *table;
/*	const char *names;
/*
/*	const char *str_name_mask(context, table, mask)
/*	const char *context;
/*	const NAME_MASK *table;
/*	int	mask;
/*
/*	const char *str_long_name_mask(context, table, mask)
/*	const char *context;
/*	const LONG_NAME_MASK *table;
/*	long	mask;
/*
/*	int	name_mask_opt(context, table, names, flags)
/*	const char *context;
/*	const NAME_MASK *table;
/*	const char *names;
/*	int	flags;
/*
/*	long	long_name_mask_opt(context, table, names, flags)
/*	const char *context;
/*	const LONG_NAME_MASK *table;
/*	const char *names;
/*	int	flags;
/*
/*	int	name_mask_delim_opt(context, table, names, delim, flags)
/*	const char *context;
/*	const NAME_MASK *table;
/*	const char *names;
/*	const char *delim;
/*	int	flags;
/*
/*	long	long_name_mask_delim_opt(context, table, names, delim, flags)
/*	const char *context;
/*	const LONG_NAME_MASK *table;
/*	const char *names;
/*	const char *delim;
/*	int	flags;
/*
/*	const char *str_name_mask_opt(buf, context, table, mask, flags)
/*	VSTRING	*buf;
/*	const char *context;
/*	const NAME_MASK *table;
/*	int	mask;
/*	int	flags;
/*
/*	const char *str_long_name_mask_opt(buf, context, table, mask, flags)
/*	VSTRING	*buf;
/*	const char *context;
/*	const LONG_NAME_MASK *table;
/*	long	mask;
/*	int	flags;
/* DESCRIPTION
/*	name_mask() takes a null-terminated \fItable\fR with (name, mask)
/*	values and computes the bit-wise OR of the masks that correspond
/*	to the names listed in the \fInames\fR argument, separated by
/*	comma and/or whitespace characters. The "long_" version returns
/*	a "long int" bitmask, rather than an "int" bitmask.
/*
/*	str_name_mask() translates a mask into its equlvalent names.
/*	The result is written to a static buffer that is overwritten
/*	upon each call. The "long_" version converts a "long int"
/*	bitmask, rather than an "int" bitmask.
/*
/*	name_mask_opt() and str_name_mask_opt() are extended versions
/*	with additional fine control. name_mask_delim_opt() supports
/*	non-default delimiter characters.
/*
/*	Arguments:
/* .IP buf
/*	Null pointer or pointer to buffer storage.
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
/* .IP delim
/*	Delimiter characters to use instead of whitespace and commas.
/* .IP flags
/*	Bit-wise OR of one or more of the following.  Where features
/*	would have conflicting results (e.g., FATAL versus IGNORE),
/*	the feature that takes precedence is described first.
/*
/*	When converting from string to mask, at least one of the
/*	following must be specified: NAME_MASK_FATAL, NAME_MASK_RETURN,
/*	NAME_MASK_WARN or NAME_MASK_IGNORE.
/*
/*	When converting from mask to string, at least one of the
/*	following must be specified: NAME_MASK_NUMBER, NAME_MASK_FATAL,
/*	NAME_MASK_RETURN, NAME_MASK_WARN or NAME_MASK_IGNORE.
/* .RS
/* .IP NAME_MASK_NUMBER
/*	When converting from string to mask, accept hexadecimal
/*	inputs starting with "0x" followed by hexadecimal digits.
/*	Each hexadecimal input may specify multiple bits.  This
/*	feature is ignored for hexadecimal inputs that cannot be
/*	converted (malformed, out of range, etc.).
/*
/*	When converting from mask to string, represent bits not
/*	defined in \fItable\fR as "0x" followed by hexadecimal
/*	digits. This conversion always succeeds.
/* .IP NAME_MASK_FATAL
/*	Require that all names listed in \fIname\fR exist in
/*	\fItable\fR or that they can be parsed as a hexadecimal
/*	string, and require that all bits listed in \fImask\fR exist
/*	in \fItable\fR or that they can be converted to hexadecimal
/*	string.  Terminate with a fatal run-time error if this
/*	condition is not met.  This feature is enabled by default
/*	when calling name_mask() or str_name_mask().
/* .IP NAME_MASK_RETURN
/*	Require that all names listed in \fIname\fR exist in
/*	\fItable\fR or that they can be parsed as a hexadecimal
/*	string, and require that all bits listed in \fImask\fR exist
/*	in \fItable\fR or that they can be converted to hexadecimal
/*	string.  Log a warning, and return 0 (name_mask()) or a
/*	null pointer (str_name_mask()) if this condition is not
/*	met.  This feature is not enabled by default when calling
/*	name_mask() or str_name_mask().
/* .IP NAME_MASK_WARN
/*	Require that all names listed in \fIname\fR exist in
/*	\fItable\fR or that they can be parsed as a hexadecimal
/*	string, and require that all bits listed in \fImask\fR exist
/*	in \fItable\fR or that they can be converted to hexadecimal
/*	string.  Log a warning if this condition is not met, continue
/*	processing, and return all valid bits or names.  This feature
/*	is not enabled by default when calling name_mask() or
/*	str_name_mask().
/* .IP NAME_MASK_IGNORE
/*	Silently ignore names listed in \fIname\fR that don't exist
/*	in \fItable\fR and that can't be parsed as a hexadecimal
/*	string, and silently ignore bits listed in \fImask\fR that
/*	don't exist in \fItable\fR and that can't be converted to
/*	hexadecimal string.
/* .IP NAME_MASK_ANY_CASE
/*	Enable case-insensitive matching.
/*	This feature is not enabled by default when calling name_mask();
/*	it has no effect with str_name_mask().
/* .IP NAME_MASK_COMMA
/*	Use comma instead of space when converting a mask to string.
/* .IP NAME_MASK_PIPE
/*	Use "|" instead of space when converting a mask to string.
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
#include <errno.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <name_mask.h>
#include <vstring.h>

static int hex_to_ulong(char *, unsigned long, unsigned long *);

#define STR(x) vstring_str(x)

/* name_mask_delim_opt - compute mask corresponding to list of names */

int     name_mask_delim_opt(const char *context, const NAME_MASK *table,
		            const char *names, const char *delim, int flags)
{
    const char *myname = "name_mask";
    char   *saved_names = mystrdup(names);
    char   *bp = saved_names;
    int     result = 0;
    const NAME_MASK *np;
    char   *name;
    int     (*lookup) (const char *, const char *);
    unsigned long ulval;

    if ((flags & NAME_MASK_REQUIRED) == 0)
	msg_panic("%s: missing NAME_MASK_FATAL/RETURN/WARN/IGNORE flag",
		  myname);

    if (flags & NAME_MASK_ANY_CASE)
	lookup = strcasecmp;
    else
	lookup = strcmp;

    /*
     * Break up the names string, and look up each component in the table. If
     * the name is found, merge its mask with the result.
     */
    while ((name = mystrtok(&bp, delim)) != 0) {
	for (np = table; /* void */ ; np++) {
	    if (np->name == 0) {
		if ((flags & NAME_MASK_NUMBER)
		    && hex_to_ulong(name, ~0U, &ulval)) {
		    result |= (unsigned int) ulval;
		} else if (flags & NAME_MASK_FATAL) {
		    msg_fatal("unknown %s value \"%s\" in \"%s\"",
			      context, name, names);
		} else if (flags & NAME_MASK_RETURN) {
		    msg_warn("unknown %s value \"%s\" in \"%s\"",
			     context, name, names);
		    myfree(saved_names);
		    return (0);
		} else if (flags & NAME_MASK_WARN) {
		    msg_warn("unknown %s value \"%s\" in \"%s\"",
			     context, name, names);
		}
		break;
	    }
	    if (lookup(name, np->name) == 0) {
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

const char *str_name_mask_opt(VSTRING *buf, const char *context,
			              const NAME_MASK *table,
			              int mask, int flags)
{
    const char *myname = "name_mask";
    const NAME_MASK *np;
    int     len;
    static VSTRING *my_buf = 0;
    int     delim = (flags & NAME_MASK_COMMA ? ',' :
		     (flags & NAME_MASK_PIPE ? '|' : ' '));

    if ((flags & STR_NAME_MASK_REQUIRED) == 0)
	msg_panic("%s: missing NAME_MASK_NUMBER/FATAL/RETURN/WARN/IGNORE flag",
		  myname);

    if (buf == 0) {
	if (my_buf == 0)
	    my_buf = vstring_alloc(1);
	buf = my_buf;
    }
    VSTRING_RESET(buf);

    for (np = table; mask != 0; np++) {
	if (np->name == 0) {
	    if (flags & NAME_MASK_NUMBER) {
		vstring_sprintf_append(buf, "0x%x%c", mask, delim);
	    } else if (flags & NAME_MASK_FATAL) {
		msg_fatal("%s: unknown %s bit in mask: 0x%x",
			  myname, context, mask);
	    } else if (flags & NAME_MASK_RETURN) {
		msg_warn("%s: unknown %s bit in mask: 0x%x",
			 myname, context, mask);
		return (0);
	    } else if (flags & NAME_MASK_WARN) {
		msg_warn("%s: unknown %s bit in mask: 0x%x",
			 myname, context, mask);
	    }
	    break;
	}
	if (mask & np->mask) {
	    mask &= ~np->mask;
	    vstring_sprintf_append(buf, "%s%c", np->name, delim);
	}
    }
    if ((len = VSTRING_LEN(buf)) > 0)
	vstring_truncate(buf, len - 1);
    VSTRING_TERMINATE(buf);

    return (STR(buf));
}

/* long_name_mask_delim_opt - compute mask corresponding to list of names */

long    long_name_mask_delim_opt(const char *context,
				         const LONG_NAME_MASK * table,
			               const char *names, const char *delim,
				         int flags)
{
    const char *myname = "name_mask";
    char   *saved_names = mystrdup(names);
    char   *bp = saved_names;
    long    result = 0;
    const LONG_NAME_MASK *np;
    char   *name;
    int     (*lookup) (const char *, const char *);
    unsigned long ulval;

    if ((flags & NAME_MASK_REQUIRED) == 0)
	msg_panic("%s: missing NAME_MASK_FATAL/RETURN/WARN/IGNORE flag",
		  myname);

    if (flags & NAME_MASK_ANY_CASE)
	lookup = strcasecmp;
    else
	lookup = strcmp;

    /*
     * Break up the names string, and look up each component in the table. If
     * the name is found, merge its mask with the result.
     */
    while ((name = mystrtok(&bp, delim)) != 0) {
	for (np = table; /* void */ ; np++) {
	    if (np->name == 0) {
		if ((flags & NAME_MASK_NUMBER)
		    && hex_to_ulong(name, ~0UL, &ulval)) {
		    result |= ulval;
		} else if (flags & NAME_MASK_FATAL) {
		    msg_fatal("unknown %s value \"%s\" in \"%s\"",
			      context, name, names);
		} else if (flags & NAME_MASK_RETURN) {
		    msg_warn("unknown %s value \"%s\" in \"%s\"",
			     context, name, names);
		    myfree(saved_names);
		    return (0);
		} else if (flags & NAME_MASK_WARN) {
		    msg_warn("unknown %s value \"%s\" in \"%s\"",
			     context, name, names);
		}
		break;
	    }
	    if (lookup(name, np->name) == 0) {
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

/* str_long_name_mask_opt - mask to string */

const char *str_long_name_mask_opt(VSTRING *buf, const char *context,
				           const LONG_NAME_MASK * table,
				           long mask, int flags)
{
    const char *myname = "name_mask";
    int     len;
    static VSTRING *my_buf = 0;
    int     delim = (flags & NAME_MASK_COMMA ? ',' :
		     (flags & NAME_MASK_PIPE ? '|' : ' '));
    const LONG_NAME_MASK *np;

    if ((flags & STR_NAME_MASK_REQUIRED) == 0)
	msg_panic("%s: missing NAME_MASK_NUMBER/FATAL/RETURN/WARN/IGNORE flag",
		  myname);

    if (buf == 0) {
	if (my_buf == 0)
	    my_buf = vstring_alloc(1);
	buf = my_buf;
    }
    VSTRING_RESET(buf);

    for (np = table; mask != 0; np++) {
	if (np->name == 0) {
	    if (flags & NAME_MASK_NUMBER) {
		vstring_sprintf_append(buf, "0x%lx%c", mask, delim);
	    } else if (flags & NAME_MASK_FATAL) {
		msg_fatal("%s: unknown %s bit in mask: 0x%lx",
			  myname, context, mask);
	    } else if (flags & NAME_MASK_RETURN) {
		msg_warn("%s: unknown %s bit in mask: 0x%lx",
			 myname, context, mask);
		return (0);
	    } else if (flags & NAME_MASK_WARN) {
		msg_warn("%s: unknown %s bit in mask: 0x%lx",
			 myname, context, mask);
	    }
	    break;
	}
	if (mask & np->mask) {
	    mask &= ~np->mask;
	    vstring_sprintf_append(buf, "%s%c", np->name, delim);
	}
    }
    if ((len = VSTRING_LEN(buf)) > 0)
	vstring_truncate(buf, len - 1);
    VSTRING_TERMINATE(buf);

    return (STR(buf));
}

/* hex_to_ulong - 0x... to unsigned long or smaller */

static int hex_to_ulong(char *value, unsigned long mask, unsigned long *ulp)
{
    unsigned long result;
    char   *cp;

    if (strncasecmp(value, "0x", 2) != 0)
	return (0);

    /*
     * Check for valid hex number. Since the value starts with 0x, strtoul()
     * will not allow a negative sign before the first nibble. So we don't
     * need to worry about explicit +/- signs.
     */
    errno = 0;
    result = strtoul(value, &cp, 16);
    if (*cp != '\0' || errno == ERANGE)
	return (0);

    if (ulp)
	*ulp = (result & mask);
    return (*ulp == result);
}

#ifdef TEST

 /*
  * Stand-alone test program.
  */
#include <stdlib.h>
#include <vstream.h>
#include <vstring_vstream.h>

int     main(int argc, char **argv)
{
    static const NAME_MASK demo_table[] = {
	"zero", 1 << 0,
	"one", 1 << 1,
	"two", 1 << 2,
	"three", 1 << 3,
	0, 0,
    };
    static const NAME_MASK feature_table[] = {
	"DEFAULT", NAME_MASK_DEFAULT,
	"FATAL", NAME_MASK_FATAL,
	"ANY_CASE", NAME_MASK_ANY_CASE,
	"RETURN", NAME_MASK_RETURN,
	"COMMA", NAME_MASK_COMMA,
	"PIPE", NAME_MASK_PIPE,
	"NUMBER", NAME_MASK_NUMBER,
	"WARN", NAME_MASK_WARN,
	"IGNORE", NAME_MASK_IGNORE,
	0,
    };
    int     in_feature_mask;
    int     out_feature_mask;
    int     demo_mask;
    const char *demo_str;
    VSTRING *out_buf = vstring_alloc(1);
    VSTRING *in_buf = vstring_alloc(1);

    if (argc != 3)
	msg_fatal("usage: %s in-feature-mask out-feature-mask", argv[0]);
    in_feature_mask = name_mask(argv[1], feature_table, argv[1]);
    out_feature_mask = name_mask(argv[2], feature_table, argv[2]);
    while (vstring_get_nonl(in_buf, VSTREAM_IN) != VSTREAM_EOF) {
	demo_mask = name_mask_opt("name", demo_table,
				  STR(in_buf), in_feature_mask);
	demo_str = str_name_mask_opt(out_buf, "mask", demo_table,
				     demo_mask, out_feature_mask);
	vstream_printf("%s -> 0x%x -> %s\n",
		       STR(in_buf), demo_mask,
		       demo_str ? demo_str : "(null)");
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(in_buf);
    vstring_free(out_buf);
    exit(0);
}

#endif
